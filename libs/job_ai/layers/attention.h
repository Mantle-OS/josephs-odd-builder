#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <cassert>

// Interfaces
#include "ilayer.h"
#include "paramgroup_type.h"
#include "paramgroup_config.h"
#include "iparamgroup.h"

#include "attention_config.h"

// Cords
#include "matrix.h"

// Comp
#include "aligned_allocator.h"
#include "attention_shape.h"
#include "gemm.h"

// Infra
#include "workspace.h"

// Adapters
#include "adapter_factory.h"
#include "adapter_ctx.h"

namespace job::ai::layers {

class Attention : public ILayer, public IParamGroup {
public:
    Attention(const AttentionConfig &cfg, int dim) :
        m_cfg(cfg),
        m_dim(dim),
        m_headDim(dim / (cfg.numHeads > 0 ? cfg.numHeads : 1)), // Safety check
        m_adapter(adapters::makeAdapter(cfg.adapterType))
    {
        //[dim x dim]: Q, K, V, Output
        size_t matrixSize = static_cast<size_t>(dim) * dim;
        size_t totalParams = matrixSize * 4;

        if (m_cfg.useBias)
            totalParams += (static_cast<size_t>(dim) * 4); // Biases for Q,K,V,O

        // Allocate contiguous blob (Aligned)
        m_storage.resize(totalParams);

        // Set Pointers into the blob
        float *ptr = m_storage.data();

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
    //////////////////////////////////////////////
    // ILayer  override
    //////////////////////////////////////////////
    [[nodiscard]] LayerType type() const noexcept override
    {
        return LayerType::Attention;
    }

    [[nodiscard]] std::string &name() noexcept override
    {
        return m_layerName;
    }

    [[nodiscard]] cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent& inputShape) const override
    {
        return inputShape; // Identity shape
    }

    void forward(threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 infer::Workspace &ws) override
    {
        using namespace job::ai::cords;
        using namespace job::ai::comp;

        // const int B = static_cast<int>(input.extent()[0]);
        // const int S = static_cast<int>(input.extent()[1]); // Sequence Length

        // FIXME

        int B = 0;
        int S = 0;
        if (input.extent().rank() == 3) {
            B = static_cast<int>(input.extent()[0]);
            S = static_cast<int>(input.extent()[1]);
            // Dim is extent()[2], which matches m_dim
        } else {
            // Fallback for flattened inputs (Assumes Batch=1 or requires S known?)
            // This is dangerous. Let's assume Rank 2 means [Batch*Seq, Dim]
            // But we can't recover S from that without external info.
            // For now, if we get Rank 2, we assume S=1 (e.g. inference step)
            B = static_cast<int>(input.extent()[0]);
            S = 1;
        }

        const int D = m_dim;
        const size_t elementsPerBuffer = static_cast<size_t>(B) * S * D;

        size_t totalFloatsNeeded = elementsPerBuffer * 4;

        if (ws.size() < totalFloatsNeeded)
            ws.resize(totalFloatsNeeded * sizeof(float));

        float *rawMem = ws.raw();
        float *qPtr = rawMem;
        float *kPtr = rawMem + elementsPerBuffer;
        float *vPtr = rawMem + (elementsPerBuffer * 2);
        float *aPtr = rawMem + (elementsPerBuffer * 3);

        // Create Views (Cheap stack objects)
        Matrix X(const_cast<float*>(input.data()), B * S, D); // Input flattened
        Matrix Q(qPtr, B * S, D);
        Matrix K(kPtr, B * S, D);
        Matrix V(vPtr, B * S, D);
        Matrix A_Out(aPtr, B * S, D);       // Adapter Output
        Matrix Final_Out(output.data(), B * S, D); // Result

        // Weights Views
        Matrix WQ(m_wq, D, D);
        Matrix WK(m_wk, D, D);
        Matrix WV(m_wv, D, D);
        Matrix WO(m_wo, D, D);

        sgemm_parallel(pool, X, WQ, Q);
        sgemm_parallel(pool, X, WK, K);
        sgemm_parallel(pool, X, WV, V);

        if (m_cfg.useBias) {
            size_t total = elementsPerBuffer;
            // Capture pointers for efficiency
            float *qp = Q.data(); const float *bq = m_bq;
            float *kp = K.data(); const float *bk = m_bk;
            float *vp = V.data(); const float *bv = m_bv;

            // Simple parallel for loop to add bias vector to every row
            job::threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
                int col = i % D;
                qp[i] += bq[col];
                kp[i] += bk[col];
                vp[i] += bv[col];
            });
        }

        adapters::AdapterCtx ctx;
        ctx.embedDim = D;
        ctx.headDim = m_headDim;

        // Need to check the config :( for verlet
        // ctx.dt = 0.01f;

        cords::AttentionShape shape;
        shape.batch = static_cast<uint32_t>(B);
        shape.seq = static_cast<uint32_t>(S);
        shape.dim = static_cast<uint32_t>(D);
        shape.numHeads = static_cast<uint32_t>(m_cfg.numHeads);

        // K -> Sources, Q -> Targets, V -> Values
        m_adapter->adapt(pool, shape, K, Q, V, A_Out, ctx);

        sgemm_parallel(pool, A_Out, WO, Final_Out);

        if (m_cfg.useBias) {
            float* out = Final_Out.data();
            const float* bo = m_bo;
            size_t total = elementsPerBuffer;

            job::threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
                out[i] += bo[i % D];
            });
        }
    }




    [[nodiscard]] std::size_t parameterCount() const noexcept  override
    {
        return m_storage.size();
    }

    [[nodiscard]] cords::ViewR parameters() noexcept override
    {
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_storage.data(), Ext(static_cast<uint32_t>(m_storage.size())));
    }

    void resetState() noexcept override
    {
        // FIXME LATER
    };


    //////////////////////////////////////////////
    // End ILayer  override
    //////////////////////////////////////////////





    //////////////////////////////////////////////
    // IParamGroup  override
    //////////////////////////////////////////////
    ParamGroup paramGroups() override
    {
        using Ext = cords::ViewR::Extent;
        ParamGroup group;

        auto add_view = [&](float* ptr, const std::string &suffix, ParamGroupType type) {
            size_t sz = (type == ParamGroupType::Bias) ? m_dim : (static_cast<size_t>(m_dim) * m_dim);
            group.push_back(ParamGroupConfig{
                .name = "attn." + suffix,
                .type = type,
                .data = cords::ViewR(ptr, Ext(static_cast<uint32_t>(sz)))
            });
        };

        add_view(m_wq, "wq", ParamGroupType::Weights);
        add_view(m_wk, "wk", ParamGroupType::Weights);
        add_view(m_wv, "wv", ParamGroupType::Weights);
        add_view(m_wo, "wo", ParamGroupType::Weights);

        if (m_cfg.useBias) {
            add_view(m_bq, "bq", ParamGroupType::Bias);
            add_view(m_bk, "bk", ParamGroupType::Bias);
            add_view(m_bv, "bv", ParamGroupType::Bias);
            add_view(m_bo, "bo", ParamGroupType::Bias);
        }
        // STCK FIXME
        return group;
    }



private:
    AttentionConfig                                         m_cfg;
    int                                                     m_dim;
    int                                                     m_headDim;
    std::unique_ptr<adapters::IAdapter>                     m_adapter;
    std::vector<float, cords::AlignedAllocator<float, 64>>  m_storage;
    float                                                   *m_wq;
    float                                                   *m_wk;
    float                                                   *m_wv;
    float                                                   *m_wo;
    float                                                   *m_bq{nullptr};
    float                                                   *m_bk{nullptr};
    float                                                   *m_bv{nullptr};
    float                                                   *m_bo{nullptr};
    std::string                                             m_layerName{"Attention"};
};

} // namespace job::ai::layers
