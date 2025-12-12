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
        const int batch = static_cast<int>(input.extent()[0]);
        const int dim = static_cast<int>(input.extent()[1]);
        const size_t outputSizeBytes = static_cast<size_t>(batch) * dim * sizeof(float);

        std::memset(output.data(), 0, outputSizeBytes);

        router::RouterPlan plan = route(pool, input, ws, nullptr);

        std::vector<std::vector<router::RouterToken>> buckets(m_cfg.numExperts);
        for (const auto &tok : plan)
            buckets[tok.expert].push_back(tok);

        size_t routeUsedFloats = (static_cast<size_t>(batch) * m_cfg.numExperts) // Logits
                                 + (plan.tokenCount * sizeof(router::RouterToken)/sizeof(float)); // Tokens

        // 16 floats -> AVX
        routeUsedFloats = (routeUsedFloats + 15) & ~15;

        float* freeMemBase = ws.raw() + routeUsedFloats;
        size_t floatsNeededPerExpert = static_cast<size_t>(batch) * dim;
        size_t totalFloatsNeeded = floatsNeededPerExpert * m_cfg.numExperts;

        // convert floats to bytes
        size_t bytesNeeded = totalFloatsNeeded * sizeof(float);
        size_t bytesAvailable = (ws.size() - routeUsedFloats) * sizeof(float);

        bool usePrivateBuffers = (bytesNeeded <= bytesAvailable);

        if (usePrivateBuffers) {
            threads::parallel_for(pool, size_t{0}, static_cast<size_t>(m_cfg.numExperts), [&](size_t eIdx) {
                const int e = static_cast<int>(eIdx);
                auto &tokens = buckets[e];
                if (tokens.empty() || !m_experts[e])
                    return;

                const int subBatch = static_cast<int>(tokens.size());

                float *localOutput = freeMemBase + (eIdx * floatsNeededPerExpert);
                // Zero it out first!
                std::memset(localOutput, 0, outputSizeBytes);

                // scratchpads for input/output (small, so vector is fine for now :>( )
                std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(subBatch * dim);
                std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(subBatch * dim);

                for (int i = 0; i < subBatch; ++i) {
                    const float *src = input.data() + tokens[i].row * dim;
                    std::memcpy(expertInput.data() + i*dim, src, dim * sizeof(float));
                }

                cords::ViewR inView(expertInput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });
                cords::ViewR outView(expertOutput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });

                // LATER pass a sub-workspace, but for now ... trust the expert.
                m_experts[e]->forward(pool, inView, outView, ws);

                // Scatter to PRIVATE buffer (No Atomics needed!)
                for (int i = 0; i < subBatch; ++i) {
                    const int row = tokens[i].row;
                    const float weight = tokens[i].weight;
                    const float *src = expertOutput.data() + i * dim;
                    float *dst = localOutput + row * dim;

                    if (weight == 1.0f){
                        for (int d = 0; d < dim; ++d)
                            dst[d] += src[d];
                    } else {
                        for (int d = 0; d < dim; ++d)
                            dst[d] += src[d] * weight;
                    }
                }
            });

            // This is memory bound, parallelize over the whole tensor [Batch * Dim]
            size_t totalElements = static_cast<size_t>(batch) * dim;

            // block size for cache locality
            size_t reduceBlock = 1024;

            threads::parallel_for(pool, size_t{0}, (totalElements + reduceBlock - 1) / reduceBlock, [&](size_t blockIdx) {
                size_t start = blockIdx * reduceBlock;
                size_t end = std::min(start + reduceBlock, totalElements);

                float *finalOut = output.data();

                // For every element in this block
                for (size_t i = start; i < end; ++i) {
                    float sum = 0.0f;
                    // Sum across all expert buffers
                    for (uint32_t e = 0; e < m_cfg.numExperts; ++e) {
                        float* expertBuf = freeMemBase + (e * floatsNeededPerExpert);
                        sum += expertBuf[i];
                    }
                    finalOut[i] = sum;
                }
            });

        } else {
            JOB_LOG_WARN("[SparseMoE] Workspace too small for private buffers. Falling back to Atomics.");
            threads::parallel_for(pool, size_t{0}, static_cast<size_t>(m_cfg.numExperts), [&](size_t eIdx) {
                const int e = static_cast<int>(eIdx);
                auto &tokens = buckets[e];

                if (tokens.empty() || !m_experts[e])
                    return;

                const int subBatch = static_cast<int>(tokens.size());
                std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(subBatch * dim);
                std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(subBatch * dim);

                for (int i = 0; i < subBatch; ++i) {
                    const float *src = input.data() + tokens[i].row * dim;
                    std::memcpy(expertInput.data() + i*dim, src, dim * sizeof(float));
                }

                cords::ViewR inView(expertInput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });
                cords::ViewR outView(expertOutput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });

                m_experts[e]->forward(pool, inView, outView, ws);

                for (int i = 0; i < subBatch; ++i) {
                    const int row = tokens[i].row;
                    const float weight = tokens[i].weight;
                    const float *src = expertOutput.data() + i * dim;
                    float *dst = output.data() + row * dim;

                    for (int d = 0; d < dim; ++d) {
                        float val = weight * src[d];
                        comp::atomicAdd(dst[d], val);
                    }
                }
            });
        }
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

        // Align up a bit (helps your AVX alignment habit)
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
