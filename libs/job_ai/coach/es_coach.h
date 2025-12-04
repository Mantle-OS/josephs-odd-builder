#pragma once

#include <vector>
#include <future>
#include <algorithm>
#include <memory>
#include <random>

#include <job_logger.h>

#include <job_thread_pool.h>

#include "icoach.h"
#include "coach_types.h"
#include "coach_config.h"
#include "mutator.h"

namespace job::ai::coach {

class ESCoach : public ICoach {
public:
    using Config = ESConfig;


    ESCoach(threads::ThreadPool::Ptr pool, Config cfg = CoachPresets::kStandard):
        m_pool(std::move(pool)),
        m_config(cfg),
        m_mutator(cfg.seed ? cfg.seed : std::random_device{}())
    {
    }

    [[nodiscard]] CoachType type() const override
    {
        return CoachType::ES;
    }

    [[nodiscard]] std::string name() const override
    {
        return "EvolutionStrategy (1, Lambda)";
    }

    void setPopulationSize(size_t size) override
    {
        m_config.populationSize = size;
    }

    void setMutationRate(float rate) override
    {
        // In ES, mutation rate is often synonymous with Sigma (strength)
        m_config.sigma = rate;
    }

    void setMode(OptimizationMode mode) override
    {
        m_optMode = mode;
    }

    [[nodiscard]] size_t generation() const override
    {
        return m_generation;
    }

    [[nodiscard]] float currentBestFitness() const override
    {
        return m_bestFitness;
    }

    evo::Genome step(const evo::Genome &parent, Evaluator eval) override
    {
        m_generation++;

        // Annealing
        if (m_config.decay < 1.0f && m_config.decay > 0.0f)
            m_config.sigma *= m_config.decay;

        // population & mutation
        std::vector<evo::Genome> population;
        population.reserve(m_config.populationSize);
        std::vector<std::future<float>> futures;
        futures.reserve(m_config.populationSize);

        for (size_t i = 0; i < m_config.populationSize; ++i) {
            population.push_back(parent);
            m_mutator.perturb(population.back(), m_config.sigma);
            futures.push_back(m_pool->submit([&, idx = i]() {
                return eval(population[idx]);
            }));
        }

        // gather results
        struct Result {
            size_t index;
            float score;
        };
        std::vector<Result> results;
        results.reserve(m_config.populationSize);

        for (size_t i = 0; i < futures.size(); ++i)
            results.push_back({i, futures[i].get()});

        // "Is A worse than B?"
        auto isWorse = [this](const Result &a, const Result &b) {
            if (m_optMode == OptimizationMode::Maximize)
                return a.score < b.score; // Higher is better
            else
                return a.score > b.score; // Lower is better
        };

        // std::max_element uses the comparator to find the "Best"
        auto bestIt = std::max_element(results.begin(), results.end(), isWorse);

        // update state
        m_bestFitness = bestIt->score;

        // move the winner out
        return std::move(population[bestIt->index]);
    }

private:
    threads::ThreadPool::Ptr    m_pool;
    Config                      m_config;
    evo::Mutator                m_mutator;
    OptimizationMode            m_optMode{OptimizationMode::Maximize};
    size_t                      m_generation{0};
    float                       m_bestFitness{0.0f};
};

} // namespace job::ai::coach
