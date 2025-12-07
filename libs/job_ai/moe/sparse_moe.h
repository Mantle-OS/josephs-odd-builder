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
#include "iparamgroup_provider.h"
#include "workspace.h" // The Zero-Alloc Memory Bank

// Compute
#include "noise_table.h"
#include "gemm.h"
#include "activation_math.h"
#include "atomic_math.h"

namespace job::ai::moe {

class SparseMoE : public IMoE, public layers::IParamGroupProvider {
public:
    SparseMoE(int inputDim, int numExperts, int topk = 2) :
        m_routerCfg(router::RouterPresets::TopK(numExperts, topk)),
        m_inputDim(inputDim),
        m_numExperts(numExperts)
    {
        // Router weights [Dim x NumExperts]
        m_gateWeights.resize(inputDim * numExperts);
        // Expert slots
        m_experts.resize(numExperts);
    }

    [[nodiscard]] layers::LayerType type() const override
    {
        return layers::LayerType::SparseMoE;
    }
    [[nodiscard]] std::string name() const override
    {
        return "SparseMoE";
    }

    void addExpert(int id, std::shared_ptr<layers::ILayer> expert) override
    {
        if (id >= 0 && id < m_numExperts)
            m_experts[id] = std::move(expert);
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
        size_t logitCount = batch * m_numExperts;
        float *logitsPtr = ws.raw();

        // Note: In a real stack, we'd advance the WS pointer.
        // For now, we assume we own the start of WS.

        // Run Router Network (TopK)
        if (m_routerCfg.type == router::RouterType::TopK) {
            cords::Matrix A(const_cast<float*>(input.data()), batch, dim);
            cords::Matrix W(m_gateWeights.data(), dim, m_numExperts);
            cords::Matrix G(logitsPtr, batch, m_numExperts);

            comp::sgemm_parallel(pool, A, W, G);

            // Noise Injection
            if (!m_routerCfg.deterministic) {
                uint64_t fastSeed = (uint64_t)input.data() ^ 0xDEADBEEF;
                comp::NoiseTable::instance().perturb(logitsPtr, logitCount, fastSeed, 1.0f);
            }
        }

        size_t maxTokens = batch * m_routerCfg.topK; // Where is this going to be used ?
        [[maybe_unused]] size_t usedBytes = (logitCount * sizeof(float)) + (maxTokens * sizeof(router::RouterToken));
        assert(usedBytes <= ws.size() * sizeof(float) && "Workspace overflow!");

        // POINTER ARITHMETIC: Skip over the logits we just wrote
        float *tokenRawPtr = logitsPtr + logitCount;

        // Cast to RouterToken*
        router::RouterToken* tokenStore = reinterpret_cast<router::RouterToken*>(tokenRawPtr);
        router::RouterPlan plan;

        switch (m_routerCfg.type) {
        case router::RouterType::TopK:
            plan = router::routeTopK(m_routerCfg, batch, m_numExperts, logitsPtr, tokenStore);
            break;
        case router::RouterType::Hash:
            plan = router::routeHash(m_routerCfg, batch, m_numExperts, input, tokenStore);
            break;
        case router::RouterType::Spatial:
            plan = router::routeSpatial(m_routerCfg, batch, input, tokenStore);
            break;
        case router::RouterType::State:
            plan = router::routeState(m_routerCfg, batch, m_numExperts, tokenStore);
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
        const int dim   = static_cast<int>(input.extent()[1]);
        const size_t outputSizeBytes = static_cast<size_t>(batch) * dim * sizeof(float);

        // 1. Zero Final Output
        std::memset(output.data(), 0, outputSizeBytes);

        // 2. Route
        router::RouterPlan plan = route(pool, input, ws, nullptr);

        // 3. Bucket
        std::vector<std::vector<router::RouterToken>> buckets(m_numExperts);
        for (const auto &tok : plan)
            buckets[tok.expert].push_back(tok);
        size_t routeUsedFloats = (static_cast<size_t>(batch) * m_numExperts) // Logits
                                 + (plan.tokenCount * sizeof(router::RouterToken)/sizeof(float)); // Tokens

        // Align to 16 floats (64 bytes) for AVX
        routeUsedFloats = (routeUsedFloats + 15) & ~15;

        float* freeMemBase = ws.raw() + routeUsedFloats;
        size_t floatsNeededPerExpert = static_cast<size_t>(batch) * dim;
        size_t totalFloatsNeeded = floatsNeededPerExpert * m_numExperts;

        // Check capacity (convert floats to bytes)
        size_t bytesNeeded = totalFloatsNeeded * sizeof(float);
        size_t bytesAvailable = (ws.size() - routeUsedFloats) * sizeof(float);

        bool usePrivateBuffers = (bytesNeeded <= bytesAvailable);

        if (usePrivateBuffers) {
            // 1. Parallel Expert Execution (Write to Private)
            threads::parallel_for(pool, size_t{0}, static_cast<size_t>(m_numExperts), [&](size_t eIdx) {
                const int e = static_cast<int>(eIdx);
                auto &tokens = buckets[e];
                if (tokens.empty() || !m_experts[e]) return;

                const int subBatch = static_cast<int>(tokens.size());

                // My Private Output Buffer in Workspace
                float* myOutput = freeMemBase + (eIdx * floatsNeededPerExpert);
                // Zero it out first! (Mandatory since we accumulate)
                std::memset(myOutput, 0, outputSizeBytes);

                // Scratchpads for Input/Output (Small, so vector is fine for now)
                std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(subBatch * dim);
                std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(subBatch * dim);

                // Gather
                for (int i = 0; i < subBatch; ++i) {
                    const float *src = input.data() + tokens[i].row * dim;
                    std::memcpy(expertInput.data() + i*dim, src, dim * sizeof(float));
                }

                // Execute
                cords::ViewR inView(expertInput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });
                cords::ViewR outView(expertOutput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });

                // Recurse (Pass ws, but expert must be careful not to clobber our private buffers)
                // Ideally we pass a sub-workspace, but for now we trust the expert.
                m_experts[e]->forward(pool, inView, outView, ws);

                // Scatter to PRIVATE buffer (No Atomics needed!)
                for (int i = 0; i < subBatch; ++i) {
                    const int row = tokens[i].row;
                    const float weight = tokens[i].weight;
                    const float *src = expertOutput.data() + i * dim;
                    float *dst = myOutput + row * dim;

                    // Vectorize this add? Compiler usually handles this small loop well.
                    if (weight == 1.0f) {
                        for (int d = 0; d < dim; ++d) dst[d] += src[d];
                    } else {
                        for (int d = 0; d < dim; ++d) dst[d] += src[d] * weight;
                    }
                }
            });

            // Reduction (Sum 8 buffers -> Final Output)
            // This is memory bound, parallelize over the whole tensor [Batch * Dim]
            size_t totalElements = static_cast<size_t>(batch) * dim;

            // We use a block size for cache locality
            size_t reduceBlock = 1024;

            threads::parallel_for(pool, size_t{0}, (totalElements + reduceBlock - 1) / reduceBlock, [&](size_t blockIdx) {
                size_t start = blockIdx * reduceBlock;
                size_t end = std::min(start + reduceBlock, totalElements);

                float* finalOut = output.data();

                // For every element in this block
                for (size_t i = start; i < end; ++i) {
                    float sum = 0.0f;
                    // Sum across all expert buffers
                    for (int e = 0; e < m_numExperts; ++e) {
                        float* expertBuf = freeMemBase + (e * floatsNeededPerExpert);
                        sum += expertBuf[i];
                    }
                    finalOut[i] = sum;
                }
            });

        } else {
            // JOB_LOG_WARN("[SparseMoE] Workspace too small for private buffers. Falling back to Atomics.");
            threads::parallel_for(pool, size_t{0}, static_cast<size_t>(m_numExperts), [&](size_t eIdx) {
                const int e = static_cast<int>(eIdx);
                auto &tokens = buckets[e];
                if (tokens.empty() || !m_experts[e]) return;

                const int subBatch = static_cast<int>(tokens.size());
                std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(subBatch * dim);
                std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(subBatch * dim);

                // Gather
                for (int i = 0; i < subBatch; ++i) {
                    const float *src = input.data() + tokens[i].row * dim;
                    std::memcpy(expertInput.data() + i*dim, src, dim * sizeof(float));
                }

                // Execute
                cords::ViewR inView(expertInput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });
                cords::ViewR outView(expertOutput.data(), cords::ViewR::Extent{ (uint32_t)subBatch, (uint32_t)dim });
                m_experts[e]->forward(pool, inView, outView, ws);

                // Scatter (ATOMIC)
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
        return inputShape; // Identity shape
    }

    layers::ParameterGroup parameterGroups() override
    {
        using PV = layers::ParamView;
        using Ext = cords::ViewR::Extent;
        layers::ParameterGroup group;

        // Gate weights
        group.push_back(PV{
            .name = "gate",
            .role = layers::ParamRole::GateWeights,
            .data = cords::ViewR(m_gateWeights.data(),
                                 Ext(static_cast<uint32_t>(m_gateWeights.size())))
        });

        // Expert parameters (Recursive)
        for (int i = 0; i < (int)m_experts.size(); ++i) {
            if (!m_experts[i])
                continue;

            if (auto *pg = dynamic_cast<layers::IParamGroupProvider*>(m_experts[i].get())) {
                auto sub = pg->parameterGroups();
                for (auto &slot : sub) {
                    // Namespacing: expert_0.weight
                    slot.name = "expert_" + std::to_string(i) + "." + slot.name;
                    slot.role = layers::ParamRole::ExpertWeights;
                    group.push_back(std::move(slot));
                }
            }
        }
        return group;
    }

    cords::ViewR parameters() override
    {
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_gateWeights.data(), Ext(static_cast<uint32_t>(m_gateWeights.size())));
    }

    size_t parameterCount() const override
    {
        return m_gateWeights.size();
    }

private:
    router::RouterConfig                                        m_routerCfg;
    [[maybe_unused]]int                                         m_inputDim{0}; // asserts only
    int                                                         m_numExperts{0};
    std::vector<float, cords::AlignedAllocator<float, 64>>      m_gateWeights;
    std::vector<std::shared_ptr<layers::ILayer>>                m_experts;
};

} // namespace job::ai::moe
