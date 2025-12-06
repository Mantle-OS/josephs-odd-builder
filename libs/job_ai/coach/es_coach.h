#pragma once

#include <vector>
#include <future>
#include <algorithm>
#include <memory>
#include <random>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <job_parallel_for.h>

#include "icoach.h"
#include "coach_types.h"
#include "coach_config.h"
#include "mutator.h"
#include "population.h"

namespace job::ai::coach {

class ESCoach : public ICoach {
public:
    using Config = ESConfig;

    ESCoach(threads::ThreadPool::Ptr pool, Config cfg = CoachPresets::kStandard) :
        m_pool(std::move(pool)),
        m_config(cfg),
        m_mutator()
    {
        // ensure population is actually sized, not just reserved
        resizePopulation(cfg.populationSize);
    }

    [[nodiscard]] CoachType type() const override { return CoachType::ES; }
    [[nodiscard]] std::string name() const override { return "EvolutionStrategy (1, Lambda)"; }

    void setPopulationSize(size_t size) override {
        m_config.populationSize = size;
        resizePopulation(size); // Resize immediately
    }

    void setMutationRate(float rate) override { m_config.sigma = rate; }
    void setMode(OptimizationMode mode) override { m_optMode = mode; }

    [[nodiscard]] size_t generation() const override { return m_generation; }
    [[nodiscard]] float currentBestFitness() const override { return m_bestFitness; }

    evo::Genome coach(const evo::Genome &parent, Evaluator eval) override
    {
        m_generation++;

        //  annealing
        if (m_config.decay < 1.0f && m_config.decay > 0.0f)
            m_config.sigma *= m_config.decay;

        const size_t popSize = m_config.populationSize;

        // Safety Check: Ensure population is sized correctly before parallel access
        if (m_population.size() != popSize) {
            resizePopulation(popSize);
        }

        threads::parallel_for(*m_pool, size_t{0}, popSize, [&](size_t i) {
            if (i >= m_population.size())
                return;

            evo::Genome &mutant = m_population.genome(i);
            mutant = parent;
            m_mutator.perturb(mutant, m_config.sigma);

            // Evaluate
            float score = eval(mutant);

            // Population::setFitnessUnsafe or similar would be better,
            // but we know size is correct now.
            m_population.setFitness(i, score);
        });

        // reduction
        size_t bestIdx = 0;
        float bestScore = m_population.getFitness(0);

        for(size_t i = 1; i < popSize; ++i) {
            float s = m_population.getFitness(i);
            bool isBetter = (m_optMode == OptimizationMode::Maximize)
                                ? (s > bestScore)
                                : (s < bestScore);

            if (isBetter) {
                bestScore = s;
                bestIdx = i;
            }
        }

        m_bestFitness = bestScore;
        return m_population.genome(bestIdx);
    }

private:
    // Helper to resize population safely
    void resizePopulation(size_t size) {
        m_population.clear();
        for(size_t i=0; i<size; ++i)
            m_population.addGenome(evo::Genome{});
        // Pre-size fitness vector inside Population (if possible via public API)
        // Since Population::addGenome pushes 0.0f to fitness, this is handled.
    }

    threads::ThreadPool::Ptr    m_pool;
    Config                      m_config;
    evo::Mutator                m_mutator;
    evo::Population             m_population;
    OptimizationMode            m_optMode{OptimizationMode::Maximize};
    size_t                      m_generation{0};
    float                       m_bestFitness{0.0f};
};

} // namespace job::ai::coach
