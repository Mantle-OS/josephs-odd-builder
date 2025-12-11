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
#include "learn_concept.h"

namespace job::ai::coach {

template <learn::Learn TLearner>
class ESCoach : public ICoach {
public:
    using Config = ESConfig;

    ESCoach(threads::ThreadPool::Ptr pool, Config cfg = CoachPresets::kStandard, TLearner learn) :
        m_pool(std::move(pool)),
        m_config(cfg),
        m_mutator(),
        m_learn(std::move(learn))
    {
        resizePopulation(cfg.populationSize);
    }

    [[nodiscard]] CoachType type() const noexcept override
    {
        return CoachType::ES;
    }
    [[nodiscard]] std::string &name() noexcept override
    {
        return m_coachName;
    }

    void setPopulationSize(size_t size) noexcept override
    {
        m_config.populationSize = size;
        resizePopulation(size);
    }

    void setMutationRate(float rate) noexcept override
    {
        m_config.sigma = rate;

    }
    void setMode(OptimizationMode mode) noexcept override
    {
        m_optMode = mode;

    }

    [[nodiscard]] size_t generation() const noexcept override
    {
        return m_generation;

    }
    [[nodiscard]] float currentBestFitness() const noexcept override
    {
        return m_bestFitness;
    }

evo::Genome coach(const evo::Genome &parent) override
{
        m_generation++;

        // Annealing
        if (m_config.decay < 1.0f && m_config.decay > 0.0f)
            m_config.sigma *= m_config.decay;

        const size_t popSize = m_config.populationSize;
        if (m_population.size() != popSize)
            resizePopulation(popSize);

        threads::parallel_for(*m_pool, size_t{0}, popSize, [&](size_t i) {
            evo::Genome &mutant = m_population.genome(i);
            mutant = parent;
            evo::Mutator localMutator;
            // evo::Mutator localMutator(m_generation * popSize + i);
            localMutator.seed(m_generation * popSize + i); // This still does not seem global !
            localMutator.perturb(mutant, m_config.sigma);

            // float score = eval(mutant); /// We get here which calls the function from     std::function<float(const evo::Genome&)> portal() which calls ->

            float score = m_learn.learn(mutant);

            m_population.setFitness(i, score);
        });

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
        m_currentBestIdx = bestIdx;
        return m_population.genome(bestIdx);
    }

    [[nodiscard]] const evo::Genome &bestGenome() const
    {
        if (m_population.size() == 0)
            throw std::runtime_error("ESCoach: No population exists yet.");

        return m_population.genome(m_currentBestIdx);
    }


private:
    void resizePopulation(size_t size) {
        m_population.clear();
        for(size_t i=0; i<size; ++i)
            m_population.addGenome(evo::Genome{});
    }

    threads::ThreadPool::Ptr    m_pool;
    Config                      m_config;
    evo::Mutator                m_mutator;
    evo::Population             m_population;
    OptimizationMode            m_optMode{OptimizationMode::Maximize};
    size_t                      m_generation{0};
    float                       m_bestFitness{0.0f};
    size_t                      m_currentBestIdx{0};
    std::string                 m_coachName{"EvolutionStrategy (1, Lambda)"};\

    TLearner                    m_learn;
};

} // namespace job::ai::coach
