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
#include "learn_factory.h"
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

        m_guiLearner = learn::makeLearner(m_config.taskType, m_pool);
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
        if (m_config.decay < 1.0f && m_config.decay > 0.0f)
            m_config.sigma *= m_config.decay;

        const size_t popSize = m_config.populationSize;
        if (m_population.size() != popSize)
            resizePopulation(popSize);

        threads::parallel_for(*m_pool, size_t{0}, popSize, [&](size_t i) {
            learn::ILearn* worker = getLearnerForThread();
            float score = -1000.0f;
            if (worker) {
                evo::Genome &mutant = m_population.genome(i);
                mutant = parent;

                evo::Mutator localMutator;
                localMutator.seed(m_generation * popSize + i);
                localMutator.perturb(mutant, m_config.sigma);

                score = worker->learn(mutant, m_config.memLimitMB);
            }
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

    [[nodiscard]] learn::ILearn* learner() override
    {
        return m_guiLearner.get();
    }

private:


    // Helper: Find or Create a learner for the calling thread
    learn::ILearn* getLearnerForThread() {
        auto id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(m_registryMutex);

        auto it = m_registry.find(id);
        if (it != m_registry.end())
            return it->second.get();

        auto newLearner = learn::makeLearner(m_config.taskType, m_pool);
        learn::ILearn* ptr = newLearner.get();
        m_registry[id] = std::move(newLearner);
        return ptr;
    }


    void resizePopulation(size_t size) {
        m_population.clear();
        for(size_t i=0; i<size; ++i)
            m_population.addGenome(evo::Genome{});
    }

    threads::ThreadPool::Ptr                                    m_pool;
    Config                                                      m_config;
    evo::Mutator                                                m_mutator;
    evo::Population                                             m_population;
    OptimizationMode                                            m_optMode{OptimizationMode::Maximize};
    size_t                                                      m_generation{0};
    float                                                       m_bestFitness{0.0f};
    size_t                                                      m_currentBestIdx{0};
    std::string                                                 m_coachName{"EvolutionStrategy (1, Lambda)"};
    std::unordered_map<std::thread::id, learn::ILearn::UPtr>    m_registry;
    std::mutex                                                  m_registryMutex;
    learn::ILearn::UPtr                                         m_guiLearner;



};

} // namespace job::ai::coach
