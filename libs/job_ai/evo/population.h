#pragma once

#include <vector>
#include <algorithm>
#include <memory>
#include <random>

#include "population_config.h"
#include "genome.h"
#include "mutator.h"
#include "crossover.h"

#include <job_random.h>

namespace job::ai::evo {

class Population {
public:
    Population() = default;

    explicit Population(const PopulationConfig &cfg) :
        m_cfg(cfg)
        , m_mutator(cfg.mutator)
        , m_crossover(cfg.crossover)
    {
        // Reserve memory but don't initialize (User or Coach must seed population)
        m_genomes.reserve(cfg.size);
        m_fitness.reserve(cfg.size);
    }

    ~Population() = default;
    Population(const Population&) = default;
    Population& operator=(const Population&) = default;
    Population(Population&&) = default;
    Population& operator=(Population&&) = default;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------
    [[nodiscard]] size_t size() const noexcept { return m_genomes.size(); }

    Genome& genome(size_t idx) { return m_genomes[idx]; }
    const Genome& genome(size_t idx) const { return m_genomes[idx]; }

    // Fitness is set externally by the Coach/Evaluator
    void setFitness(size_t idx, float val) {
        if (m_fitness.size() <= idx) m_fitness.resize(idx + 1);
        m_fitness[idx] = val;
    }

    [[nodiscard]] float getFitness(size_t idx) const { return m_fitness[idx]; }

    // -------------------------------------------------------------------------
    // Population Management
    // -------------------------------------------------------------------------
    void clear() {
        m_genomes.clear();
        m_fitness.clear();
    }

    void addGenome(Genome g) {
        m_genomes.push_back(std::move(g));
        m_fitness.push_back(0.0f); // Placeholder
    }

    // -------------------------------------------------------------------------
    // Evolution Step (The Genetic Algorithm Logic)
    // -------------------------------------------------------------------------
    // This replaces the current population with the Next Generation.
    // Note: This runs on the Main Thread (usually), acting as the reducer.
    void evolveNextGeneration() {
        if (m_genomes.empty()) return;

        // 1. Sort indices by fitness (Descending)
        std::vector<size_t> indices(m_genomes.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return m_fitness[a] > m_fitness[b];
        });

        std::vector<Genome> nextGen;
        nextGen.reserve(m_cfg.size);

        // 2. Elitism (Keep the best)
        for(size_t i=0; i < m_cfg.eliteCount && i < indices.size(); ++i) {
            nextGen.push_back(m_genomes[indices[i]]);
        }

        // 3. Reproduction (Selection + Crossover + Mutation)
        // Use Top 50% as gene pool
        size_t poolSize = std::max<size_t>(1, indices.size() / 2);

        // Use the Thread-Local Crypto RNG
        auto& rng = job::crypto::JobRandom::engine();
        std::uniform_int_distribution<size_t> dist(0, poolSize - 1);

        while (nextGen.size() < m_cfg.size) {
            // Pick Parents
            size_t idx1 = indices[dist(rng)];
            size_t idx2 = indices[dist(rng)];

            // Crossover
            Genome child = m_crossover.cross(m_genomes[idx1], m_genomes[idx2]);

            // Mutate
            // Pass the sigma from config (Population owns the 'Base' config)
            m_mutator.perturb(child, m_cfg.mutator.weightSigma);

            nextGen.push_back(std::move(child));
        }

        // 4. Swap
        m_genomes = std::move(nextGen);
        // Reset fitness buffer (size matches new genome count, values 0)
        m_fitness.assign(m_genomes.size(), 0.0f);
    }

private:
    PopulationConfig    m_cfg;
    Mutator             m_mutator;
    Crossover           m_crossover;

    // Structure of Arrays (SoA) for cache-friendly sorting
    std::vector<Genome> m_genomes;
    std::vector<float>  m_fitness;
};

} // namespace job::ai::evo
