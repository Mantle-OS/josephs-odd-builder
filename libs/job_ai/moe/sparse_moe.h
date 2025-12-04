#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <cstring>

#include "imoe.h"
#include "moe_types.h"

#include "matrix.h"
#include "aligned_allocator.h"
#include "gemm.h"
#include "activation_math.h"

namespace job::ai::moe {

class SparseMoE : public IMoE {
public:
    SparseMoE(int inputDim, int numExperts, int topk = 2) :
        m_inputDim(inputDim),
        m_numExperts(numExperts),
        m_topK(topk)
    {
        m_gateWeights.resize(inputDim * numExperts);
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

    // Expert Management
    void addExpert(int id, std::shared_ptr<layers::ILayer> expert) override
    {
        if (id >= 0 && id < m_numExperts)
            m_experts[id] = std::move(expert);
    }

    void setRouterType(RouterType type) override
    {
        m_routerType = type;
    }
    void setLoadBalancing(LoadBalanceStrategy strategy) override
    {
        m_loadBalancing = strategy;
    }


    // forward traffic cop
    void forward( job::threads::ThreadPool &pool, const cords::ViewR &input, cords::ViewR &output) override
    {
        int batch = input.extent()[0];
        int dim   = input.extent()[1];

        // compute gating logits: gate = input * gateweights
        // [batch, dim] * [dim, experts] -> [batch, experts]
        // we need a scratch buffer for logits
        std::vector<float> gateLogits(batch * m_numExperts);

        cords::Matrix A(const_cast<float*>(input.data()), batch, dim);
        cords::Matrix W(m_gateWeights.data(), dim, m_numExperts);
        cords::Matrix G(gateLogits.data(), batch, m_numExperts);

        // run the router nn
        comp::sgemm_parallel(pool, A, W, G);

        // routing (top-k selection)
        // FIXME make parallelized, but sequential
        // helper struct to sort scores
        struct TokenRoute {
            int expertIdx;
            float weight;
        };

        // map: [expertid] -> list of {rowindexinbatch, weight}
        // We need to group tokens so we can send them to experts in bulk
        std::vector<std::vector<std::pair<int, float>>> expertBatching(m_numExperts);
        for(auto& vec : expertBatching)
            vec.reserve(batch / m_numExperts * 2); // Heuristic reserve

        for (int b = 0; b < batch; ++b) {
            float* rowLogits = gateLogits.data() + b * m_numExperts;

            // softmax (partial/topk approximation) or just raw scores ?
            // standard moe applies softmax over the topk.

            // find top k indices
            // FIXME simple partial sort for now (optimization target!)
            std::vector<std::pair<float, int>> scores(m_numExperts);
            for(int e=0; e<m_numExperts; ++e)
                scores[e] = {rowLogits[e], e};

            std::partial_sort(scores.begin(), scores.begin() + m_topK, scores.end(),[](const auto &a, const auto &b){
                return a.first > b.first;
            });

            // Softmax denominator for just the Top K
            float maxVal = scores[0].first;
            float sumExp = 0.0f;
            for(int k = 0; k < m_topK; ++k)
                sumExp += std::exp(scores[k].first - maxVal);

            // Assign to Expert Buckets
            for(int k = 0; k < m_topK; ++k) {
                int expertId = scores[k].second;
                float prob = std::exp(scores[k].first - maxVal) / sumExp;
                expertBatching[expertId].push_back({b, prob});
            }
        }

        // dispatch & execute (parallel across experts)
        // each expert runs on its specific subset of data.
        // FIXME: atm we must copy data to contiguous buffers for the experts (gemm requirement :>( ).

        // need some sorta mechanism to store results before recombination.
        // output is pre-allocated. i can accumulate into it maybe ? or write to scratch and sum. since topk > 1, multiple experts write to same row.
        // safe bet: atomic add? or reduction buffer?
        // fast bet: zero output, then mutex-protected add? or lock-free stride? i am going with this life in the fast lane !

        // zero the output first
        std::memset(output.data(), 0, output.size() * sizeof(float));

        // parallelize over expertsiterate expert indices, not tasks, to spawn jobs
        std::vector<std::future<void>> futures;

        for(int e = 0; e < m_numExperts; ++e) {
            if (expertBatching[e].empty()) continue;

            futures.push_back(pool.submit([this, e, &expertBatching, &input, &output, &pool]() {
                const auto& tokens = expertBatching[e];
                int subBatchSize = static_cast<int>(tokens.size());

                // copy inputs to contiguous scratchpad
                // allocating inside the loop?
                // FIXME In prod, use a thread-local scratch pool.
                std::vector<float, cords::AlignedAllocator<float, 64>> expertInput(subBatchSize * m_inputDim);
                std::vector<float, cords::AlignedAllocator<float, 64>> expertOutput(subBatchSize * m_inputDim); // FIXME DimIn == DimOut for now

                for(int i=0; i<subBatchSize; ++i) {
                    int originalRow = tokens[i].first;
                    std::memcpy(expertInput.data() + i * m_inputDim,
                                input.data() + originalRow * m_inputDim,
                                m_inputDim * sizeof(float));
                }

                // execute expert
                cords::ViewR inView(expertInput.data(),
                                    cords::ViewR::Extent{static_cast<uint32_t>(subBatchSize), static_cast<uint32_t>(m_inputDim)}
                                    );
                cords::ViewR outView(expertOutput.data(),
                                     cords::ViewR::Extent{static_cast<uint32_t>(subBatchSize), static_cast<uint32_t>(m_inputDim)}
                                     );

                if (m_experts[e])
                    m_experts[e]->forward(pool, inView, outView);

                // scatter / recombine (weighted add)

                // CRITICAL Section: multiple experts might write to the same output row (if TopK > 1)
                // We need a lock per row, or atomic floats.

                // HACK for V1: one global lock (Slow) or atomic CAS loop.
                // better: Since TopK is small (2), probability of collision is low? No.
                // correct approach: sharded locks on rows?

                // use a spin-lock on rows? Or just a mutex for the output block.
                // just lock the whole recombination for now and optimize later.
                static std::mutex globalMux;
                std::lock_guard<std::mutex> lock(globalMux);

                for(int i=0; i<subBatchSize; ++i) {
                    int originalRow = tokens[i].first;
                    float weight = tokens[i].second;

                    float* dest = output.data() + originalRow * m_inputDim;
                    const float* src = expertOutput.data() + i * m_inputDim;

                    // vectorized weighted add
                    for(int d=0; d<m_inputDim; ++d)
                        dest[d] += src[d] * weight;
                }
            }));
        }

        // Wait for all experts
        for(auto& f : futures)
            f.get();
    }

    // routing introspection
    std::vector<int> route([[maybe_unused]]const cords::ViewR &input) override
    {
        // simplified version of logic above, just returns indices
        // TODO: reuse the logic to avoid duplication
        return {};
    }

    cords::ViewR parameters() override
    {
        // This is tricky for MoE. It's a collection of models.
        // probably return the gate weights only here ? Or(leaning this way) we need a "ParameterGroup" concept.
        using Ext = cords::ViewR::Extent;
        return cords::ViewR(m_gateWeights.data(), Ext(static_cast<uint32_t>(m_gateWeights.size())));
    }

    size_t parameterCount() const override
    {
        return m_gateWeights.size(); // + experts size?
    }

private:
    int                                                     m_inputDim{0};
    int                                                     m_numExperts{0};
    int                                                     m_topK{0};
    RouterType                                              m_routerType{RouterType::TopK};
    LoadBalanceStrategy                                     m_loadBalancing{LoadBalanceStrategy::None};
    // router weights
    std::vector<float, cords::AlignedAllocator<float, 64>>  m_gateWeights;
    // experts(dense layers, etc.)
    std::vector<std::shared_ptr<layers::ILayer>>            m_experts;
};

} // namespace job::ai::moe
