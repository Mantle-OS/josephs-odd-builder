#pragma once

#include <vector>
#include <algorithm>
#include <future>
#include <memory>

#include <job_thread_pool.h>
#include "genome.h"
#include "mutator.h"

namespace job::ai::coach {

using namespace job::threads;
using namespace job::ai::base;
using namespace job::ai::evo;

class ESTrainer {
public:
    struct Config {
        size_t population_size          = 100;
        core::real_t sigma              = core::real_t(0.02);   // Mutation strength
        core::real_t elite_fraction     = core::real_t(0.1);    // Top 10% survive
    };

    ESTrainer(std::shared_ptr<ThreadPool> pool, Config config) :
        m_pool(pool),
        m_config(config),
        m_mutator(std::random_device{}())
    {

    }

    // The Main Loop
    // 1. Mutate parent -> Offspring
    // 2. Evaluate (User provided callback)
    // 3. Select & Update
    void step(Genome &parent, std::function<core::real_t(const Genome&)> evaluate_fn)
    {

        // Spawn Offspring
        std::vector<Genome> population(m_config.population_size);
        std::vector<std::future<core::real_t>> futures;
        futures.reserve(m_config.population_size);

        // Keep the parent as index 0 (Elitism)
        population[0] = parent;

        // Generate Mutants
        for(size_t i=1; i<m_config.population_size; ++i) {
            population[i] = parent; // Copy
            // Apply Mutation (Perturb weights)
            // m_mutator.perturb(population[i].weights, m_config.sigma);
            // TODO: Apply Structural Mutation (Add/Remove LayerGene)
        }

        // Parallel Evaluation
        for(size_t i=0; i<m_config.population_size; ++i) {
            futures.push_back(m_pool->submit([&, i]() {
                return evaluate_fn(population[i]);
            }));
        }

        // gather results
        std::vector<std::pair<core::real_t, size_t>> scores;
        for(size_t i = 0; i < futures.size(); ++i) {
            core::real_t fitness = futures[i].get();
            scores.push_back({fitness, i});
            population[i].header.fitness = fitness;
        }

        // selection (Rank based)
        std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
            return a.first > b.first; // Descending (Higher is better)
        });

        // Update Parent (Simplest Strategy: Winner takes all)
        // In full CMA-ES, we would compute a weighted average of the top k.
        // For simple ES, we just take the best mutant.
        size_t best_idx = scores[0].second;
        parent = std::move(population[best_idx]);
    }

private:
    ThreadPool::Ptr     m_pool;
    Config              m_config;
    Mutator             m_mutator;
};

} // namespace job::ai::coach
