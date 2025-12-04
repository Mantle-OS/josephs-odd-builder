#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

// Interfaces
#include "imoe.h"
#include "moe_types.h"

// Router
#include "router_configs.h"
#include "router_impl.h"

// cords
#include "matrix.h"
// layers
#include "iparamgroup_provider.h"

// comp
#include "aligned_allocator.h"
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
        m_gateWeights.resize(inputDim * numExperts);
        m_experts.resize(numExperts);
    }

    layers::ParameterGroup parameterGroups() override {
        using PV = layers::ParamView;
        using Ext = cords::ViewR::Extent;
        layers::ParameterGroup group;

        //  gate weights
        group.push_back(PV{
            .name = "gate",
            .role = layers::ParamRole::GateWeights,
            .data = cords::ViewR(m_gateWeights.data(),
                                 Ext(static_cast<uint32_t>(m_gateWeights.size())))
        });

        // recursively gather params
        for (int i = 0; i < (int)m_experts.size(); ++i) {
            if (!m_experts[i])
                continue;

            if (auto *pg = dynamic_cast<layers::IParamGroupProvider*>(m_experts[i].get())) {
                auto sub = pg->parameterGroups();
                for (auto &slot : sub) {
                    // Namespacing: expert_0.weight, expert_1.bias
                    slot.name = "expert_" + std::to_string(i) + "." + slot.name;
                    slot.role = layers::ParamRole::ExpertWeights;
                    group.push_back(std::move(slot));
                }
            }
        }
        return group;
    }


    [[nodiscard]] layers::LayerType type() const override
    {
        return layers::LayerType::SparseMoE;
    }
    [[nodiscard]] std::string name() const override
    {
        return "SparseMoE";
    }

    void addExpert(int id, std::shared_ptr<layers::ILayer> expert) override {
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

    router::RouterPlan route(threads::ThreadPool &pool, const cords::ViewR &input, std::vector<float> *maybeGateLogits) override
    {
        const int batch = input.extent()[0];
        const int dim   = input.extent()[1];
        assert(dim == m_inputDim);

        std::vector<float> localLogits;
        float *logitsPtr = nullptr;

        // run router network (if topk)
        if (m_routerCfg.type == router::RouterType::TopK) {
            localLogits.resize(batch * m_numExperts);

            cords::Matrix A(const_cast<float*>(input.data()), batch, dim);
            cords::Matrix W(m_gateWeights.data(), dim, m_numExperts);
            cords::Matrix G(localLogits.data(), batch, m_numExperts);

            comp::sgemm_parallel(pool, A, W, G);
            logitsPtr = localLogits.data();
        }

        router::RouterPlan plan;
        plan.numExperts = m_numExperts;
        plan.batchSize  = batch;

        // routes
        switch (m_routerCfg.type) {
        case router::RouterType::TopK:
            plan = router::routeTopK(m_routerCfg, plan, logitsPtr);
            break;
        case router::RouterType::Hash:
            plan = router::routeHash(m_routerCfg, plan, input);
            break;
        case router::RouterType::Spatial:
            plan = router::routeSpatial(m_routerCfg, plan, input);
            break;
        case router::RouterType::State:
            plan = router::routeState(m_routerCfg, plan, input);
            break;
        }

        if (maybeGateLogits && logitsPtr) *maybeGateLogits = std::move(localLogits);
        return plan;
    }

    void forward(job::threads::ThreadPool &pool,
                 const cords::ViewR &input,
                 cords::ViewR &output) override
    {
        const int batch = static_cast<int>(input.extent()[0]);
        const int dim   = static_cast<int>(input.extent()[1]);
        assert(dim == m_inputDim);

        // zero output (atomic adds accumulate here)
        std::memset(output.data(), 0, static_cast<size_t>(batch) * dim * sizeof(float));

        // route
        router::RouterPlan plan = route(pool, input, nullptr);

        // bucket tokens (group by expert)
        std::vector<std::vector<router::RouterToken>> buckets(m_numExperts);
        for (const auto &tok : plan.tokens) buckets[tok.expert].push_back(tok);

        // parallel expert execution

        threads::parallel_for(pool, size_t{0}, static_cast<size_t>(m_numExperts), [&](size_t eIdx) {
            const int e = static_cast<int>(eIdx);
            auto &tokens = buckets[e];
            if (tokens.empty() || !m_experts[e])
                return;

            const int subBatch = static_cast<int>(tokens.size());

            // scratch buffers (thread-local allocation for now)
            // FIXME: in v2, use a pre-allocated workspace passed in context
            std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(static_cast<size_t>(subBatch) * dim);
            std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(static_cast<size_t>(subBatch) * dim);

            // copy input -> scratch
            for (int i = 0; i < subBatch; ++i) {
                const int row = tokens[i].row;
                const float *src = input.data() + row * dim;
                float *dst       = expertInput.data() + i * dim;
                std::memcpy(dst, src, static_cast<size_t>(dim) * sizeof(float));
            }

            // execute expert
            cords::ViewR inView(expertInput.data(), cords::ViewR::Extent{ static_cast<uint32_t>(subBatch), static_cast<uint32_t>(dim) });
            cords::ViewR outView(expertOutput.data(), cords::ViewR::Extent{ static_cast<uint32_t>(subBatch), static_cast<uint32_t>(dim) });

            m_experts[e]->forward(pool, inView, outView);

            // scatter  recombine (weighted atomic add
            for (int i = 0; i < subBatch; ++i) {
                const int   row    = tokens[i].row;
                const float weight = tokens[i].weight;

                const float *src = expertOutput.data() + i * dim;
                float *dst       = output.data() + row * dim;

                for (int d = 0; d < dim; ++d) {
                    float val = weight * src[d];
                    comp::atomicAdd(dst[d], val);
                }
            }
        });
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
    router::RouterConfig                                    m_routerCfg;            // the config of the router
    int                                                     m_inputDim{0};          // the iunput dim
    int                                                     m_numExperts{0};        // how many expers are there ?
    std::vector<float, cords::AlignedAllocator<float, 64>>  m_gateWeights;          // the router's own weights (if topk is used)
    std::vector<std::shared_ptr<layers::ILayer>>            m_experts;              // "society" of experts
};

} // namespace job::ai::moe
