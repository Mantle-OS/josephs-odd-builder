#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <cassert>

// Core Crypto/Random
#include <job_random.h>

// Interfaces
#include "imoe.h"
#include "moe_types.h"

// Router
#include "router_configs.h"
#include "router_impl.h"

// Cords & Memory
#include "matrix.h"
#include "aligned_allocator.h"

// Layers & Infer
#include "abstract_layer.h"
#include "iparamgroup.h"
#include "workspace.h" // The Zero-Alloc Memory Bank

// Compute
#include "noise_table.h"
#include "gemm.h"
#include "activation_math.h"
#include "atomic_math.h"
#include "workspace_arena.h"

namespace job::ai::moe {
using namespace job::ai::layers;
class SparseMoE final : public IMoE, public AbstractLayer,  public layers::IParamGroup {
public:
    SparseMoE(uint32_t inputDim, uint32_t numExperts, uint32_t topk = 2, LayerConfig cfg = LayerPresets::MoEConfig(), float alpha = 0.0f) :
        layers::AbstractLayer{cfg, alpha},
        m_routerCfg(router::RouterPresets::TopK(numExperts, topk)),
        m_inputDim(inputDim)
    {
        m_cfg.numExperts = numExperts;
        m_cfg.topK = topk;

        m_gateWeights.resize(inputDim * numExperts);
        m_experts.resize(numExperts);
    }

    void addExpert(uint32_t id, AbstractLayer::Ptr expert) override
    {
        if (id < m_cfg.numExperts)
            m_experts[id] = std::move(expert);
    }

    [[nodiscard]] layers::LayerType type() const noexcept override
    {
        return layers::LayerType::SparseMoE;
    }

    void setRouterType(router::RouterType type) override
    {
        m_routerCfg.type = type;
    }

    void setLoadBalancing(router::LoadBalanceStrategy strategy) override
    {
        m_routerCfg.lbStrategy = strategy;
    }

    router::RouterPlan route(threads::ThreadPool &pool, const cords::ViewR &input,
                             infer::Workspace &ws, std::vector<float> *maybeGateLogits) override
    {
        const int batch = input.extent()[0];
        const int dim   = input.extent()[1];
        size_t logitCount = batch * m_cfg.numExperts;
        float *logitsPtr = ws.raw();

        // Note: In a "real stack", I'd advance the WS pointer. For now, assume I own the start of WS.

        // TopK
        if (m_routerCfg.type == router::RouterType::TopK) {
            cords::Matrix A(const_cast<float*>(input.data()), batch, dim);
            cords::Matrix W(m_gateWeights.data(), dim, m_cfg.numExperts);
            cords::Matrix G(logitsPtr, batch, m_cfg.numExperts);

            comp::sgemm_parallel(pool, A, W, G);

            // noise
            if (!m_routerCfg.deterministic) {
                uint64_t fastSeed = (uint64_t)input.data() ^ 0xDEADBEEF;
                comp::NoiseTable::instance().perturb(logitsPtr, logitCount, fastSeed, 1.0f);
            }
        }

        size_t maxTokens = batch * m_routerCfg.topK;
        [[maybe_unused]] size_t usedBytes = (logitCount * sizeof(float)) + (maxTokens * sizeof(router::RouterToken));
        assert(usedBytes <= ws.size() * sizeof(float) && "Workspace overflow!");

        float *tokenRawPtr = logitsPtr + logitCount;

        // cast to RouterToken*
        router::RouterToken* tokenStore = reinterpret_cast<router::RouterToken*>(tokenRawPtr);
        router::RouterPlan plan;

        switch (m_routerCfg.type) {
        case router::RouterType::TopK:
            plan = router::routeTopK(m_routerCfg, batch, m_cfg.numExperts, logitsPtr, tokenStore);
            break;
        case router::RouterType::Hash:
            plan = router::routeHash(m_routerCfg, batch, m_cfg.numExperts, input, tokenStore);
            break;
        case router::RouterType::Spatial:
            plan = router::routeSpatial(m_routerCfg, batch, input, tokenStore);
            break;
        case router::RouterType::State:
            plan = router::routeState(m_routerCfg, batch, m_cfg.numExperts, tokenStore);
            break;
        }

        if (maybeGateLogits) {
            maybeGateLogits->resize(logitCount);
            std::memcpy(maybeGateLogits->data(), logitsPtr, logitCount * sizeof(float));
        }

        return plan;
    }
    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 infer::Workspace &ws) override
    {
        const uint32_t B = input.extent()[0];
        const uint32_t D = input.extent()[1];
        const uint32_t E = m_cfg.numExperts;
        const uint32_t K = m_cfg.topK;

        std::memset(output.data(), 0, std::size_t(B) * D * sizeof(float));

        // Route (assume route writes logits+tokens into ws and returns non-owning plan.tokens)
        auto plan = route(pool, input, ws, nullptr);
        const uint32_t T = (uint32_t)plan.tokenCount;

        // Arena starts AFTER routing storage (logits + tokens)
        infer::WorkspaceArena arena{ws};
        arena.cursorBytes = infer::WorkspaceArena::align_up(
            std::size_t(B) * E * sizeof(float) + std::size_t(T) * sizeof(router::RouterToken),
            64
            );

        float *expertIn  = arena.allocFloats(std::size_t(T) * D);
        float *expertOut = arena.allocFloats(std::size_t(T) * D);
        if (!expertIn || !expertOut) {
            // fallback path (atomics) or enlarge ws
            // ...
            return;
        }

        std::vector<uint32_t> counts(E, 0);
        for (uint32_t t = 0; t < T; ++t)
            counts[plan.tokens[t].expert]++;

        std::vector<uint32_t> expertBase(E + 1, 0);
        for (uint32_t e = 0; e < E; ++e)
            expertBase[e + 1] = expertBase[e] + counts[e];

        std::vector<uint32_t> writeHead(E);
        for (uint32_t e = 0; e < E; ++e)
            writeHead[e] = expertBase[e];

        std::vector<uint32_t> tokenIdxByExpert(T);
        std::vector<uint32_t> localIndex(T);

        for (uint32_t t = 0; t < T; ++t) {
            uint32_t e = (uint32_t)plan.tokens[t].expert;
            uint32_t pos = writeHead[e]++;
            tokenIdxByExpert[pos] = t;
            localIndex[t] = pos - expertBase[e];
        }

        // Per-row token list (<=K). Avoid atomics at the end.
        std::vector<uint32_t> rowCount(B, 0);
        std::vector<uint32_t> rowTok(std::size_t(B) * K, 0xFFFFFFFF);

        for (uint32_t t = 0; t < T; ++t) {
            uint32_t r = (uint32_t)plan.tokens[t].row;
            uint32_t c = rowCount[r]++;
            if (c < K)
                rowTok[std::size_t(r) * K + c] = t;
        }

        // Experts in parallel (safe ONLY if experts don't use ws internally)
        threads::parallel_for(pool, size_t{0}, size_t(E), [&](size_t eIdx) {
            const uint32_t e = (uint32_t)eIdx;
            if (!m_experts[e] || counts[e] == 0)
                return;

            const uint32_t nTok  = counts[e];
            const uint32_t start = expertBase[e];

            for (uint32_t i = 0; i < nTok; ++i) {
                const uint32_t t = tokenIdxByExpert[start + i];
                const uint32_t r = (uint32_t)plan.tokens[t].row;
                std::memcpy(expertIn + std::size_t(start + i) * D,
                            input.data() + std::size_t(r) * D,
                            std::size_t(D) * sizeof(float));
            }

            cords::ViewR inV (expertIn  + std::size_t(start) * D, cords::ViewR::Extent{nTok, D});
            cords::ViewR outV(expertOut + std::size_t(start) * D, cords::ViewR::Extent{nTok, D});

            m_experts[e]->forward(pool, inV, outV, ws);
        });

        // Final combine (parallel over rows, no full reduction, no atomics)
        threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t r) {
            float* dst = output.data() + r * D;
            // dst already zeroed; keep ?????

            const uint32_t cnt = rowCount[(uint32_t)r];
            for (uint32_t j = 0; j < cnt && j < K; ++j) {
                const uint32_t t = rowTok[r*K + j];
                if (t == 0xFFFFFFFF)
                    continue;

                const uint32_t e   = (uint32_t)plan.tokens[t].expert;
                const uint32_t idx = expertBase[e] + localIndex[t];
                const float* src   = expertOut + std::size_t(idx) * D;
                const float w       = plan.tokens[t].weight;

                if (w == 1.0f)
                    for (uint32_t d = 0; d < D; ++d)
                        dst[d] += src[d];
                else
                    for (uint32_t d = 0; d < D; ++d)
                        dst[d] += src[d] * w;
            }
        });
    }

    cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const override
    {
        return inputShape;
    }

    layers::ParamGroup paramGroups() override
    {
        using Ext = cords::ViewR::Extent;
        layers::ParamGroup group;
        // gates
        group.push_back(layers::ParamGroupConfig{
            .name = "gate",
            .type = layers::ParamGroupType::GateWeights,
            .data = cords::ViewR(m_gateWeights.data(),
                                 Ext(static_cast<uint32_t>(m_gateWeights.size())))
        });

        for (int i = 0; i < (int)m_experts.size(); ++i) {
            if (!m_experts[i])
                continue;

            if (auto *pg = dynamic_cast<layers::IParamGroup*>(m_experts[i].get())) {
                auto sub = pg->paramGroups();
                for (auto &slot : sub) {
                    slot.name = "expert_" + std::to_string(i) + "." + slot.name;
                    slot.type = layers::ParamGroupType::ExpertWeights;
                    group.push_back(std::move(slot));
                }
            }
        }
        return group;
    }

    cords::ViewR parameters() noexcept override
    {
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_gateWeights.data(), Ext(static_cast<uint32_t>(m_gateWeights.size())));
    }

    size_t parameterCount() const noexcept override
    {
        return m_gateWeights.size();
    }


    void loadWeights(float *weights) override
    {
        float *p = weights;
        const size_t gateCount = size_t(m_inputDim) * m_cfg.numExperts;
        std::memcpy(m_gateWeights.data(), p, gateCount * sizeof(float));
        p += gateCount;

        for (uint32_t i = 0; i < m_cfg.numExperts; ++i) {
            if (!m_experts[i])
                continue;

            m_experts[i]->loadWeights(p);

            // advance by what that expert expects
            p += m_experts[i]->parameterCount();
        }
    }

    // the size of the scratch pad.
    [[nodiscard]] size_t scratchSize(const cords::ViewR::Extent &extent) const override
    {
        // Expect [batch, dim]
        const uint32_t batch = extent.rank() >= 1 ? extent[0] : 0;
        const uint32_t dim   = extent.rank() >= 2 ? extent[1] : 0;

        const size_t logitsBytes = size_t(batch) * m_cfg.numExperts * sizeof(float);
        const size_t tokensBytes = size_t(batch) * m_cfg.topK * sizeof(router::RouterToken);

        size_t bytes = logitsBytes + tokensBytes;
        bytes = (bytes + 63) & ~size_t(63);

        // B) include it if we WANT the fast path
        // scratchSize to mean "minimum required" ? Then we can drop that
        const size_t privateBytes =
            size_t(batch) * dim * m_cfg.numExperts * sizeof(float);

        return bytes + privateBytes;
    }

    // Reset any internal state (KV cache, running stats, RNG state, etc.). when I get here ....
    void resetState() noexcept override
    {
            // FIXME LATER NOT NOW.
    }


private:
    router::RouterConfig                                        m_routerCfg;
    [[maybe_unused]]uint32_t                                    m_inputDim{0}; // asserts only
    std::vector<float, cords::AlignedAllocator<float, 64>>      m_gateWeights;
    std::vector<AbstractLayer::Ptr>                             m_experts;
};

} // namespace job::ai::moe
