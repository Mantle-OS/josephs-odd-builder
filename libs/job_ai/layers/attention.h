#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <cassert>

// Interfaces
#include "abstract_layer.h"
#include "layer_config.h"
#include "paramgroup_type.h"
#include "paramgroup_config.h"
#include "iparamgroup.h"

#include "layer_config.h"

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

class AttentionLayer final : public AbstractLayer, public IParamGroup {
public:
    AttentionLayer(int dim, const LayerConfig &cfg = LayerPresets::DenseConfig(), float alpha = 1.0f) :
        AbstractLayer{cfg, alpha},
        m_dim(dim)
    {
        m_adapter = adapters::makeAdapter(cfg.adapterType);
        int numHeads = (cfg.numHeads > 0) ? cfg.numHeads : 1;
        assert(dim % numHeads == 0 && "AttentionLayer: dim must be divisible by numHeads");
        m_headDim = dim / numHeads;

        //[dim x dim]: Q, K, V, Output
        std::size_t matrixSize = static_cast<std::size_t>(dim) * dim;
        std::size_t totalParams = matrixSize * 4;

        if (m_cfg.hasBias())
            totalParams += (static_cast<size_t>(dim) * 4); // Biases for Q,K,V,O

        // Allocate contiguous blob (Aligned)
        m_storage.resize(totalParams);

        // Set Pointers into the blob
        float *ptr = m_storage.data();

        m_wq = ptr;
        ptr += matrixSize;

        m_wk = ptr;
        ptr += matrixSize;

        m_wv = ptr;
        ptr += matrixSize;

        m_wo = ptr;
        ptr += matrixSize;

        if (m_cfg.hasBias()) {
            m_bq = ptr;
            ptr += dim;

            m_bk = ptr;
            ptr += dim;

            m_bv = ptr;
            ptr += dim;

            m_bo = ptr;
            ptr += dim;
        }
    }
    //////////////////////////////////////////////
    // AbstractLayer  override
    //////////////////////////////////////////////
    [[nodiscard]] LayerType type() const noexcept override
    {
        return m_cfg.type;
    }

    [[nodiscard]] cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const override
    {
        // dim % numHeads == 0 when numHeads > 0

        return inputShape; // Identity shape
    }

    void forward(threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 infer::Workspace &ws) override
    {
        const auto &ext = input.extent();
        const auto rank = ext.rank();

        std::uint32_t B = 0;
        std::uint32_t S = 0;
        std::uint32_t D = 0;

        if (rank == 3) {
            // [B, S, D]
            B = ext[0];
            S = ext[1];
            D = ext[2];
        } else if (rank == 2) {
            // [S, D] -> implicit B=1 (single sequence)
            B = 1;
            S = ext[0];
            D = ext[1];
        } else {
            // Anything else is a misuse of this layer
            assert(false && "AttentionLayer::forward expects [B,S,D] or [S,D]");
        }

        // Sanity: layer’s configured dim must match tensor dim
        // assert(D == static_cast<std::uint32_t>(m_dim) && "AttentionLayer: input dim != m_dim");

        // Output must have the same shape as input (semantic: context vector per token)
        // If the caller passed an "empty" view, let us define its shape.
        if (output.extent().rank() == 0)
            output = cords::ViewR(output.data(), cords::makeBSD(B, S, D));
        // else
            // assert(output.extent() == cords::makeBSD(B, S, D) && "AttentionLayer: output shape must match [B,S,D]");

        const std::size_t elementsPerBuffer =
            static_cast<std::size_t>(B) * static_cast<std::size_t>(S) * static_cast<std::size_t>(D);

        const std::size_t totalFloatsNeeded = elementsPerBuffer * 4; // e.g. Q, K, V, O
        const std::size_t bytesNeeded = totalFloatsNeeded * sizeof(float);
        if (ws.size() < bytesNeeded)
            ws.resize(bytesNeeded);

        float *rawMem = ws.raw();
        float *qPtr = rawMem;
        float *kPtr = rawMem + elementsPerBuffer;
        float *vPtr = rawMem + (elementsPerBuffer * 2);
        float *aPtr = rawMem + (elementsPerBuffer * 3);

        // Create Views (Cheap stack objects)
        cords::Matrix X(const_cast<float*>(input.data()), B * S, D); // Input flattened
        cords::Matrix Q(qPtr, B * S, D);
        cords::Matrix K(kPtr, B * S, D);
        cords::Matrix V(vPtr, B * S, D);
        cords::Matrix A_Out(aPtr, B * S, D);       // Adapter Output
        cords::Matrix Final_Out(output.data(), B * S, D); // Result

        // Weights Views
        cords::Matrix WQ(m_wq, D, D);
        cords::Matrix WK(m_wk, D, D);
        cords::Matrix WV(m_wv, D, D);
        cords::Matrix WO(m_wo, D, D);

        comp::sgemmMatrix( X, WQ, Q);
        comp::sgemmMatrix(X, WK, K);
        comp::sgemmMatrix(X, WV, V);

        if (m_cfg.hasBias()) {
            size_t total = elementsPerBuffer;
            // Capture pointers for efficiency
            float *qp = Q.data(); const float *bq = m_bq;
            float *kp = K.data(); const float *bk = m_bk;
            float *vp = V.data(); const float *bv = m_bv;

            // Simple parallel for loop to add bias vector to every row
            threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
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
        m_adapter->adaptParallel(pool, shape, K, Q, V, A_Out, ctx);

        comp::sgemmMatrix(A_Out, WO, Final_Out);

        if (m_cfg.hasBias()) {
            float *out = Final_Out.data();
            const float* bo = m_bo;
            size_t total = elementsPerBuffer;

            threads::parallel_for(pool, size_t{0}, total, [=](size_t i) {
                out[i] += bo[i % D];
            });
        }
    }


    [[nodiscard]] std::size_t parameterCount() const noexcept override
    {
        const std::size_t matrixSize = std::size_t(m_dim) * m_dim;
        std::size_t total = matrixSize * 4;
        if (m_cfg.hasBias())
            total += std::size_t(m_dim) * 4;
        return total;
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



    void loadWeights(float *weights) override final
    {
        std::memcpy(m_storage.data(),
                    weights,
                    parameterCount() * sizeof(float));

        // Rebuild the internal pointer slices in case parameterCount changed
        float *ptr = m_storage.data();
        const std::size_t matrixSize = std::size_t(m_dim) * std::size_t(m_dim);

        m_wq = ptr; ptr += matrixSize;
        m_wk = ptr; ptr += matrixSize;
        m_wv = ptr; ptr += matrixSize;
        m_wo = ptr; ptr += matrixSize;

        if (m_cfg.hasBias()) {
            m_bq = ptr; ptr += m_dim;
            m_bk = ptr; ptr += m_dim;
            m_bv = ptr; ptr += m_dim;
            m_bo = ptr; ptr += m_dim;
        } else {
            m_bq = m_bk = m_bv = m_bo = nullptr;
        }
    }


    [[nodiscard]] size_t scratchSize(const cords::ViewR::Extent &in) const override final
    {
        uint32_t B = 0, S = 0, D = 0;

        if (in.rank() == 3) {
            B = in[0];
            S = in[1];
            D = in[2];
        } else if (in.rank() == 2) {
            B = 1;
            S = in[0];
            D = in[1];
        } else {
            assert(false && "AttentionLayer::scratchSize expects [B,S,D] or [S,D]");
            return 0;
        }

        assert(D == static_cast<uint32_t>(m_dim));

        const std::size_t floats = std::size_t(B) * std::size_t(S) * std::size_t(D) * 4; // Q,K,V,O
        return floats * sizeof(float);
    }




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

        if (m_cfg.hasBias()) {
            add_view(m_bq, "bq", ParamGroupType::Bias);
            add_view(m_bk, "bk", ParamGroupType::Bias);
            add_view(m_bv, "bv", ParamGroupType::Bias);
            add_view(m_bo, "bo", ParamGroupType::Bias);
        }
        // FIXME .....Later
        return group;
    }



private:
    int                                                     m_dim;
    int                                                     m_headDim;
    std::unique_ptr<adapters::IAdapter>                     m_adapter;
    std::vector<float, core::AlignedAllocator<float, 64>>  m_storage;
    float                                                   *m_wq;
    float                                                   *m_wk;
    float                                                   *m_wv;
    float                                                   *m_wo;
    float                                                   *m_bq{nullptr};
    float                                                   *m_bk{nullptr};
    float                                                   *m_bv{nullptr};
    float                                                   *m_bo{nullptr};
};

} // namespace job::ai::layers
