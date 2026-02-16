#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <cassert>

// Core
#include <aligned_allocator.h>

// Crypto
#include <job_random.h>

// Interfaces
#include "imoe.h"
#include "moe_types.h"

// Router
#include "router_configs.h"
#include "router_impl.h"

// Cords & Memory
#include "matrix.h"

// Layers & Infer
#include "abstract_layer.h"
#include "iparamgroup.h"
#include "workspace.h"
#include "workspace_arena.h"
#include "workspace_view.h"

// Compute
#include "noise_table.h"
#include "gemm.h"
#include "atomic_math.h"

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

        // Cold Start Allocation
        m_gateWeights.resize(inputDim * numExperts);
        m_gateWeightsPtr = m_gateWeights.data(); // Point to internal

        m_experts.resize(numExperts);
    }

    void setLoadBalancing(router::LoadBalanceStrategy strategy) override
    {
        m_routerCfg.lbStrategy = strategy;
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

    router::RouterPlan route([[maybe_unused]]job::threads::ThreadPool &pool,
                             const cords::ViewR &input,
                             infer::Workspace &ws,
                             std::vector<float> *maybeGateLogits) override
    {
        assert(input.extent().rank() == 2 && "SparseMoE::route expects [B, D] for now");
        const int batch   = (int)input.extent()[0];

        const int dim     = (int)input.extent()[1];
        assert(dim == (int)m_inputDim && "SparseMoE: router input dim != m_inputDim");

        const int experts = (int)m_cfg.numExperts;

        const std::size_t logitCount = (std::size_t)batch * (std::size_t)experts;
        const std::size_t logitsBytes = logitCount * sizeof(float);

        // capacity we promise to the router
        // TopK is clamped internally to 16 in routeTopK()
        const int kCap = (m_routerCfg.type == router::RouterType::TopK) ?
                             std::min((int)m_routerCfg.topK, 16) :
                             (int)m_routerCfg.topK;

        const std::size_t tokenCap = (std::size_t)batch * (std::size_t)kCap;
        const std::size_t tokensBytesCap = tokenCap * sizeof(router::RouterToken);

        // Layout inside ws
        std::byte *base = reinterpret_cast<std::byte*>(ws.raw());
        float *logitsPtr = reinterpret_cast<float*>(base);

        // align token start
        std::size_t tokenOffset = infer::WorkspaceArena::align_up(logitsBytes, alignof(router::RouterToken));
        router::RouterToken *tokenStore = reinterpret_cast<router::RouterToken*>(base + tokenOffset);

        // total reserved bytes
        std::size_t reservedBytes = tokenOffset + tokensBytesCap;

        // Ensure ws big enough (optional: assert only, or grow once per forward)
        if (ws.size() < reservedBytes) {
            ws.resize(reservedBytes);
            base = reinterpret_cast<std::byte*>(ws.raw());
            logitsPtr = reinterpret_cast<float*>(base);
            tokenStore = reinterpret_cast<router::RouterToken*>(base + tokenOffset);
            // ws.resize preserves tokenOffset validity because tokenOffset is an integer byte offset
        }

        // TopK logits generation (writes logitsPtr[logitCount])
        if (m_routerCfg.type == router::RouterType::TopK) {
            cords::Matrix A(const_cast<float*>(input.data()), batch, dim);

            // USE THE HOT POINTER
            cords::Matrix W(m_gateWeightsPtr, dim, experts);
            cords::Matrix G(logitsPtr, batch, experts);

            comp::sgemmParallelMatrix(pool,A, W, G); // HERE IS WHAT UPDATED

            if (!m_routerCfg.deterministic) {
                uint64_t fastSeed = (uint64_t)input.data() ^ 0xDEADBEEF;
                comp::NoiseTable::instance().perturb(logitsPtr, logitCount, fastSeed, 1.0f);
            }
        }

        router::RouterPlan plan;
        switch (m_routerCfg.type) {
        case router::RouterType::TopK:
            plan = router::routeTopK(m_routerCfg, batch, experts, logitsPtr, tokenStore);
            break;
        case router::RouterType::Hash:
            plan = router::routeHash(m_routerCfg, batch, experts, input, tokenStore);
            break;
        case router::RouterType::Spatial:
            plan = router::routeSpatial(m_routerCfg, batch, input, tokenStore);
            break;
        case router::RouterType::State:
            plan = router::routeState(m_routerCfg, batch, experts, tokenStore);
            break;
        }

        // optional debug sanity: plan.tokenCount must fit the reserved buffer
        assert(plan.tokenCount <= tokenCap && "Router wrote more tokens than capacity!");

        if (maybeGateLogits) {
            maybeGateLogits->resize(logitCount);
            std::memcpy(maybeGateLogits->data(), logitsPtr, logitsBytes);
        }

        return plan;
    }


    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output,
                 infer::Workspace &ws) override
    {

        assert(input.extent().rank() == 2 &&
               "SparseMoE expects [B, D] for now");

        const uint32_t B = input.extent()[0];
        const uint32_t D = input.extent()[1];
        const uint32_t E = m_cfg.numExperts;

        const int kCap = (m_routerCfg.type == router::RouterType::TopK) ?
                             std::min<int>(m_routerCfg.topK, 16) : int(m_routerCfg.topK);

        const uint32_t T_max = B * kCap;

        const std::size_t logitsBytes = std::size_t(B) * E * sizeof(float);
        const std::size_t tokenOffset = infer::WorkspaceArena::align_up(logitsBytes, alignof(router::RouterToken));
        const std::size_t routeBytes  = tokenOffset + (std::size_t(T_max) * sizeof(router::RouterToken));

        const std::size_t expertInBytes  = std::size_t(T_max) * D * sizeof(float);
        const std::size_t expertOutBytes = std::size_t(T_max) * D * sizeof(float);

        std::size_t maxExpertScratch = 0;
        for(auto &ex : m_experts)
            if(ex)
                maxExpertScratch = std::max(maxExpertScratch, ex->scratchSize({T_max, D}));

        maxExpertScratch = infer::WorkspaceArena::align_up(maxExpertScratch, 64);

        const std::size_t totalBytesNeeded =
            infer::WorkspaceArena::align_up(routeBytes, 64) +
            infer::WorkspaceArena::align_up(expertInBytes, 64) +
            infer::WorkspaceArena::align_up(expertOutBytes, 64) +
            (std::size_t(E) * maxExpertScratch);

        if (ws.size() < totalBytesNeeded)
            ws.resize(totalBytesNeeded);

        std::memset(output.data(), 0, std::size_t(B) * D * sizeof(float));

        auto plan = route(pool, input, ws, nullptr);
        const uint32_t T = static_cast<uint32_t>(plan.tokenCount);

        infer::WorkspaceArena arena{ws};

        // Skip past the routing data
        arena.cursorBytes = infer::WorkspaceArena::align_up(routeBytes, 64);

        float *expertIn  = arena.allocFloats(std::size_t(T) * D);
        float *expertOut = arena.allocFloats(std::size_t(T) * D);

        // scratch window base
        std::byte *scratchBase = nullptr;
        if (maxExpertScratch > 0) {
            scratchBase = arena.allocBytes(std::size_t(E) * maxExpertScratch, 64);
        }

        // CSR: Histogram -> Prefix Sum -> Index Fill
        std::vector<uint32_t> counts(E, 0);
        for (uint32_t t = 0; t < T; ++t)
            counts[(uint32_t)plan.tokens[t].expert]++;

        std::vector<uint32_t> expertBase(E + 1, 0);
        for (uint32_t e = 0; e < E; ++e)
            expertBase[e + 1] = expertBase[e] + counts[e];

        std::vector<uint32_t> writeHead = expertBase;
        std::vector<uint32_t> tokenIdxByExpert(T);
        std::vector<uint32_t> packedPosOfToken(T);

        for (uint32_t t = 0; t < T; ++t) {
            const uint32_t e = (uint32_t)plan.tokens[t].expert;
            const uint32_t pos = writeHead[e]++;
            tokenIdxByExpert[pos] = t;
            packedPosOfToken[t] = pos;
        }

        threads::parallel_for(pool, size_t{0}, size_t(E), [&](size_t eIdx) {
            const uint32_t e = (uint32_t)eIdx;
            const uint32_t nTok = counts[e];
            if (!m_experts[e] || nTok == 0)
                return;

            const uint32_t start = expertBase[e];
            float *myIn  = expertIn  + std::size_t(start) * D;
            float *myOut = expertOut + std::size_t(start) * D;

            // Gather
            for (uint32_t i = 0; i < nTok; ++i) {
                const uint32_t t = tokenIdxByExpert[start + i];
                const uint32_t r = (uint32_t)plan.tokens[t].row;
                std::memcpy(myIn + std::size_t(i) * D,
                            input.data() + std::size_t(r) * D,
                            std::size_t(D) * sizeof(float));
            }

            cords::ViewR inV (myIn,  cords::ViewR::Extent{nTok, D});
            cords::ViewR outV(myOut, cords::ViewR::Extent{nTok, D});

            if (scratchBase) {
                // This prevents the expert from seeing (or ruining) the rest of the workspace.
                float *myScratch = reinterpret_cast<float*>(scratchBase + std::size_t(e) * maxExpertScratch);
                infer::Workspace localWs(myScratch, maxExpertScratch);
                m_experts[e]->forward(pool, inV, outV, localWs);
            } else {
                // No scratch needed (or Dense layer), pass root (safe-ish) or empty view
                // Ideally passing 'ws' is safe if maxExpertScratch was 0.
                m_experts[e]->forward(pool, inV, outV, ws);
            }
        });

        // CSR for Rows
        std::vector<uint32_t> rowCount(B, 0);
        std::vector<uint32_t> rowTok(std::size_t(B) * kCap, 0xFFFFFFFFu);

        for (uint32_t t = 0; t < T; ++t) {
            const uint32_t r = (uint32_t)plan.tokens[t].row;
            const uint32_t c = rowCount[r]++;
            if (c < (uint32_t)kCap)
                rowTok[std::size_t(r) * kCap + c] = t;
        }

        threads::parallel_for(pool, size_t{0}, size_t(B), [&](size_t rr) {
            const uint32_t r = (uint32_t)rr;
            const uint32_t cnt = rowCount[r];
            if (cnt == 0)
                return;

            float *dst = output.data() + std::size_t(r) * D;

            for (uint32_t j = 0; j < cnt && j < (uint32_t)kCap; ++j) {
                const uint32_t t = rowTok[std::size_t(r) * kCap + j];
                if (t == 0xFFFFFFFFu)
                    continue;

                const uint32_t pos = packedPosOfToken[t];
                const float *src = expertOut + std::size_t(pos) * D;
                const float w = plan.tokens[t].weight;

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

        // Router Flywheel: Swap the pointer!
        m_gateWeightsPtr = p;

        // Advance past router weights
        const size_t gateCount = size_t(m_inputDim) * m_cfg.numExperts;
        p += gateCount;

        // Recursive Load
        for (uint32_t i = 0; i < m_cfg.numExperts; ++i) {
            if (!m_experts[i])
                continue;

            // Pass the hot pointer to the expert (it will swap its own internals)
            m_experts[i]->loadWeights(p);

            // Advance by what that expert consumes
            p += m_experts[i]->parameterCount();
        }
    }


    size_t scratchSize(const cords::ViewR::Extent &extent) const override
    {
        assert(extent.rank() == 2 &&
               "SparseMoE::scratchSize expects [B,D] for now");

        const uint32_t B = extent[0];
        const uint32_t D = extent[1];
        const uint32_t E = m_cfg.numExperts;

        const int kCap = (m_routerCfg.type == router::RouterType::TopK) ?
                             std::min<int>(m_routerCfg.topK, 16) :
                             int(m_routerCfg.topK);

        const uint32_t T_max = B * kCap;
        const size_t logitsBytes = size_t(B) * E * sizeof(float);
        const size_t tokenOffset = infer::WorkspaceArena::align_up(logitsBytes, alignof(router::RouterToken));
        const size_t routeBytes  =
            tokenOffset + size_t(T_max) * sizeof(router::RouterToken);

        const size_t expertInBytes  = size_t(T_max) * D * sizeof(float);
        const size_t expertOutBytes = size_t(T_max) * D * sizeof(float);

        size_t maxExpertScratch = 0;
        for (auto &ex : m_experts) {
            if (!ex)
                continue;
            maxExpertScratch = std::max(
                maxExpertScratch,
                ex->scratchSize(cords::ViewR::Extent{T_max, D})
                );
        }
        maxExpertScratch = infer::WorkspaceArena::align_up(maxExpertScratch, 64);

        const size_t totalBytes =
            infer::WorkspaceArena::align_up(routeBytes, 64) +
            infer::WorkspaceArena::align_up(expertInBytes, 64) +
            infer::WorkspaceArena::align_up(expertOutBytes, 64) +
            size_t(E) * maxExpertScratch;

        return totalBytes;
    }


    // [[nodiscard]] size_t scratchSize(const cords::ViewR::Extent &extent) const override
    // {
    //     const uint32_t batch = extent.rank() >= 1 ? extent[0] : 0;
    //     const uint32_t dim   = extent.rank() >= 2 ? extent[1] : 0;

    //     const size_t logitsBytes = size_t(batch) * m_cfg.numExperts * sizeof(float);

    //     // Clamp K (Matches forward logic)
    //     const int kCap = (m_routerCfg.type == router::RouterType::TopK) ?
    //                          std::min<int>(m_routerCfg.topK, 16) : int(m_routerCfg.topK);

    //     const size_t tokensBytes = size_t(batch) * kCap * sizeof(router::RouterToken);

    //     size_t bytes = logitsBytes + tokensBytes;
    //     bytes = (bytes + 63) & ~size_t(63);

    //     // OPTIMIZATION: Only reserve space for active tokens (TopK), not all Experts
    //     // Need Input + Output buffers for the active tokens
    //     const size_t ioBytes = size_t(batch) * kCap * dim * 2 * sizeof(float);

    //     // I think I am lost .........
    //     // also need scratch windows for the experts ?
    //     // I can't easily know maxExpertScratch here without iterating experts, .....
    //     // so a safe overestimate is (Batch * TopK * D) * Experts? No..... too big ?
    //     // stick to tjhe safe logic or just add a margin ?

    //     // const size_t privateBytes = size_t(batch) * dim * m_cfg.numExperts * sizeof(float);

    //     // Tighter logic matching forward(): Heuristic margin
    //     return bytes + ioBytes + (ioBytes / 2);
    // }

    // Reset any internal state (KV cache, running stats, RNG state, etc.). when I get here ....
    void resetState() noexcept override
    {
            // FIXME LATER NOT NOW.
    }


private:
    router::RouterConfig                                        m_routerCfg;
    [[maybe_unused]]uint32_t                                    m_inputDim{0}; // asserts only
    std::vector<float, core::AlignedAllocator<float, 64>>      m_gateWeights;
    float                                                       *m_gateWeightsPtr{nullptr};
    std::vector<AbstractLayer::Ptr>                             m_experts;
};

} // namespace job::ai::moe
