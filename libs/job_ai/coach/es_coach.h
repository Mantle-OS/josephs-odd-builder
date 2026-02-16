#pragma once

#include <vector>
#include <future>
#include <algorithm>
#include <memory>
#include <random>
#include <mutex>
#include <thread>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <job_parallel_for.h>

#include "icoach.h"
#include "coach_types.h"
#include "coach_config.h"
#include "learn_factory.h"
// #include "mutator.h" // REMOVED: Coach doesn't need to know about this anymore
#include "population.h"

namespace job::ai::coach {

class ESCoach : public ICoach {
public:
    using Ptr = std::shared_ptr<ESConfig>;

    ESCoach(threads::ThreadPool::Ptr pool,
            ESConfig cfg = CoachPresets::kStandard,
            layers::LayerConfig layerCfg = layers::LayerPresets::DenseConfig()) :
        m_pool(std::move(pool)),
        m_config(cfg),
        m_population(evo::PopulationConfig{
            .populationSize = cfg.populationSize,
            .mutator = {
                .weightSigma = cfg.sigma,
                .weightMutationProb = 1.0f // Ensure we default to dense mutation for ES
            }
        }),
        m_layerConfig(layerCfg)
    {
        m_guiLearner = learn::makeLearner(m_config.envConfig, m_pool);
        if (m_population.size() != m_config.populationSize)
            m_population.resize(m_config.populationSize);
    }

    [[nodiscard]] CoachType type() const noexcept override { return CoachType::ES; }
    [[nodiscard]] std::string &name() noexcept override { return m_coachName; }

    void setPopulationSize(size_t size) noexcept override {
        m_config.populationSize = size;
        m_population.resize(size);
    }

    void setMutationRate(float rate) noexcept override {
        m_config.sigma = rate;
        m_population.setSigma(rate);
    }

    void setMode(OptimizationMode mode) noexcept override { m_optMode = mode; }
    [[nodiscard]] size_t generation() const noexcept override { return m_generation; }
    [[nodiscard]] float currentBestFitness() const noexcept override { return m_bestFitness; }


    evo::Genome coach(const evo::Genome &parent) override
    {
        m_generation++;
        if (m_config.decay < 1.0f && m_config.decay > 0.0f) {
            m_config.sigma *= m_config.decay;
            m_population.setSigma(m_config.sigma);
        }

        if (m_population.size() != m_config.populationSize)
            m_population.resize(m_config.populationSize);

        constexpr size_t kBatchSize = 32;
        const size_t popSize = m_population.size();
        const size_t numBatches = (popSize + kBatchSize - 1) / kBatchSize;

        threads::parallel_for(*m_pool, size_t{0}, numBatches, [&](size_t batchIdx) {

            learn::ILearn *learner = getLearnerForThread();
            if (!learner) return;
            size_t start = batchIdx * kBatchSize;
            size_t end = std::min(start + kBatchSize, popSize);
            for (size_t i = start; i < end; ++i) {
                uint64_t seed = m_generation * popSize + i;
                m_population.spawnMutant(i, parent, seed);

                const evo::Genome& child = m_population.genome(i);
                float score = learner->learn(child);

                m_population.setFitness(i, score);
            }
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

        // Return a copy of the winner to be the parent of the next generation
        return m_population.genome(bestIdx);
    }

    [[nodiscard]] const evo::Genome &bestGenome() const {
        if (m_population.size() == 0) throw std::runtime_error("No population.");
        return m_population.genome(m_currentBestIdx);
    }

    [[nodiscard]] learn::ILearn *learner() override {
        return m_guiLearner.get();
    }

    // Helper passthroughs if you need manual manipulation
    // void prependGenome(const evo::Genome &genome) { /* Not implemented in new Pop yet */ }
    // void appendGenome(const evo::Genome &genome)  { /* Not implemented in new Pop yet */ }

private:
    int m_deb_cnt = 0;

    // Helper: Find or Create a learner for the calling thread
    learn::ILearn* getLearnerForThread() {
        auto id = std::this_thread::get_id();

        // Fast Check
        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            auto it = m_registry.find(id);
            if (it != m_registry.end()) return it->second.get();
        }

        // Slow Create
        auto newLearner = learn::makeLearner(m_config.envConfig, m_pool);
        learn::ILearn *ptr = newLearner.get();

        {
            std::lock_guard<std::mutex> lock(m_registryMutex);
            m_registry[id] = std::move(newLearner);
        }

        JOB_LOG_DEBUG("Spawning new Learner (Total: {})", ++m_deb_cnt);
        return ptr;
    }

    threads::ThreadPool::Ptr         m_pool;
    ESConfig                         m_config;
    evo::Population                  m_population;

    OptimizationMode                 m_optMode{OptimizationMode::Maximize};
    size_t                           m_generation{0};
    float                            m_bestFitness{0.0f};
    size_t                           m_currentBestIdx{0};
    std::string                      m_coachName{"EvolutionStrategy (Batched)"};

    std::unordered_map<std::thread::id, learn::ILearn::UPtr> m_registry;
    std::mutex                       m_registryMutex;
    learn::ILearn::UPtr              m_guiLearner;
    layers::LayerConfig              m_layerConfig;
};

} // namespace job::ai::coach
