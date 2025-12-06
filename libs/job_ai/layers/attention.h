#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstring>

// Interfaces
#include "ilayer.h"
#include "iparamgroup_provider.h"
#include "attention_config.h"

// Core
#include "matrix.h"
#include "aligned_allocator.h"
#include "attention_shape.h"
#include "gemm.h"
#include "activation_math.h"


// Adapters
#include "adapter_factory.h"
#include "adapter_ctx.h"

namespace job::ai::layers {

class Attention : public ILayer, public IParamGroupProvider {
public:
    Attention(const AttentionConfig &cfg, int dim) :
        m_cfg(cfg),
        m_dim(dim),
        m_headDim(dim / cfg.numHeads),
        m_adapter(adapters::makeAdapter(cfg.adapterType))
    {
        // 1. Calculate Storage Size
        // 4 Matrices of [dim x dim]: Q, K, V, Output
        size_t matrixSize = static_cast<size_t>(dim) * dim;
        size_t totalParams = matrixSize * 4;

        if (m_cfg.useBias) {
            totalParams += (dim * 4); // Biases for Q,K,V,O
        }

        // 2. Allocate contiguous blob (Aligned)
        m_storage.resize(totalParams);

        // 3. Set Pointers into the blob
        float* ptr = m_storage.data();

        m_wq = ptr; ptr += matrixSize;
        m_wk = ptr; ptr += matrixSize;
        m_wv = ptr; ptr += matrixSize;
        m_wo = ptr; ptr += matrixSize;

        if (m_cfg.useBias) {
            m_bq = ptr; ptr += dim;
            m_bk = ptr; ptr += dim;
            m_bv = ptr; ptr += dim;
            m_bo = ptr; ptr += dim;
        }
    }


    [[nodiscard]] LayerType type() const override { return LayerType::Attention; }
    [[nodiscard]] std::string name() const override { return "Attention"; }


    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR& input,
                 cords::ViewR& output) override
    {
        using namespace job::ai::cords;
        using namespace job::ai::comp;

        const int B = static_cast<int>(input.extent()[0]);
        const int S = static_cast<int>(input.extent()[1]); // Sequence Length
        const int D = m_dim;

        // Scratchpad Allocation (V1: Per-call allocation)
        // Note: In V2, we pass a Workspace.
        size_t bufferSize = static_cast<size_t>(B) * S * D;

        std::vector<float, AlignedAllocator<float, 64>> qBuf(bufferSize);
        std::vector<float, AlignedAllocator<float, 64>> kBuf(bufferSize);
        std::vector<float, AlignedAllocator<float, 64>> vBuf(bufferSize);
        std::vector<float, AlignedAllocator<float, 64>> attnBuf(bufferSize);

        // Views for GEMM
        Matrix X(const_cast<float*>(input.data()), B * S, D); // Treat [B,S] as one long batch
        Matrix Q(qBuf.data(), B * S, D);
        Matrix K(kBuf.data(), B * S, D);
        Matrix V(vBuf.data(), B * S, D);
        Matrix A_Out(attnBuf.data(), B * S, D);
        Matrix Final_Out(output.data(), B * S, D);

        // Weights Views
        Matrix WQ(m_wq, D, D);
        Matrix WK(m_wk, D, D);
        Matrix WV(m_wv, D, D);
        Matrix WO(m_wo, D, D);

        // 1. Projections: Q = X * Wq, etc.
        // Parallelizing these 3 GEMMs is possible, but sgemm_parallel already saturates cores.
        sgemm_parallel(pool, X, WQ, Q);
        sgemm_parallel(pool, X, WK, K);
        sgemm_parallel(pool, X, WV, V);

        // Optional: Add Bias (Parallel Loop)
        if (m_cfg.useBias) {
            size_t total = B * S * D;
            // Capture pointers locally for [=] capture efficiency
            float* qp = Q.data(); const float* bq = m_bq;
            float* kp = K.data(); const float* bk = m_bk;
            float* vp = V.data(); const float* bv = m_bv;

            job::threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
                int col = i % D;
                qp[i] += bq[col];
                kp[i] += bk[col];
                vp[i] += bv[col];
            });
        }

        // 2. Prepare Adapter Context
        adapters::AdapterCtx ctx;
        ctx.embedDim = D;
        ctx.headDim = m_headDim;
        // ctx.dt = 0.01f; // For Verlet

        cords::AttentionShape shape;
        shape.batch = B;
        shape.seq = S;
        shape.dim = D;
        shape.numHeads = m_cfg.numHeads;

        // 3. Run Attention (The "Physics" Step)
        // Q -> Targets
        // K -> Sources
        // V -> Values (Payload)
        // A_Out -> Forces/Context
        m_adapter->adapt(pool, shape, K, Q, V, A_Out, ctx);

        // 4. Output Projection: Out = AttnOut * Wo
        sgemm_parallel(pool, A_Out, WO, Final_Out);

        // Optional: Output Bias
        if (m_cfg.useBias) {
            float* out = Final_Out.data();
            const float* bo = m_bo;
            job::threads::parallel_for(pool, size_t{0}, static_cast<size_t>(B*S*D), [=](size_t i) {
                out[i] += bo[i % D];
            });
        }
    }

    // -------------------------------------------------------------------------
    // Parameter Management
    // -------------------------------------------------------------------------
    ParameterGroup parameterGroups() override {
        using PV = ParamView;
        using Ext = cords::ViewR::Extent;
        ParameterGroup group;

        auto add_view = [&](float* ptr, const std::string& suffix, ParamRole role) {
            // D*D for weights, D for bias
            size_t sz = (role == ParamRole::Bias) ? m_dim : (m_dim * m_dim);
            group.push_back(PV{
                .name = "attn." + suffix,
                .role = role,
                .data = cords::ViewR(ptr, Ext(static_cast<uint32_t>(sz)))
            });
        };

        add_view(m_wq, "wq", ParamRole::Weights);
        add_view(m_wk, "wk", ParamRole::Weights);
        add_view(m_wv, "wv", ParamRole::Weights);
        add_view(m_wo, "wo", ParamRole::Weights);

        if (m_cfg.useBias) {
            add_view(m_bq, "bq", ParamRole::Bias);
            add_view(m_bk, "bk", ParamRole::Bias);
            add_view(m_bv, "bv", ParamRole::Bias);
            add_view(m_bo, "bo", ParamRole::Bias);
        }

        return group;
    }

    cords::ViewR parameters() override {
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_storage.data(), Ext(static_cast<uint32_t>(m_storage.size())));
    }

    size_t parameterCount() const override {
        return m_storage.size();
    }

private:
    AttentionConfig m_cfg;
    int m_dim;
    int m_headDim;

    // The Strategy (Flash vs FMM vs Verlet)
    std::unique_ptr<adapters::IAdapter> m_adapter;

    // Memory Blob
    std::vector<float, cords::AlignedAllocator<float, 64>> m_storage;

    // Pointers into blob
    float *m_wq, *m_wk, *m_wv, *m_wo;
    float *m_bq{nullptr}, *m_bk{nullptr}, *m_bv{nullptr}, *m_bo{nullptr};
};

} // namespace job::ai::layers
