#pragma once

#include <cassert>
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

class Population final {
public:
    Population() = default;

    explicit Population(const PopulationConfig &cfg) :
        m_cfg(cfg),
        m_mutator(cfg.mutator),
        m_crossover(cfg.crossover)
    {
        m_genomes.resize(cfg.populationSize);
        m_fitness.resize(cfg.populationSize, 0.0f);
    }


    void spawnMutant(std::size_t index, const Genome& parent, uint64_t seed)
    {
        assert(index < m_genomes.size());
        m_genomes[index] = parent;
        Mutator localMutator = m_mutator;
        localMutator.seed(seed);
        localMutator.perturb(m_genomes[index]);
    }


    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_genomes.size();
    }

    Genome &genome(std::size_t index)
    {
        assert(index < m_genomes.size());
        return m_genomes[index];
    }

    const Genome &genome(std::size_t index) const
    {
        assert(index < m_genomes.size());
        return m_genomes[index];
    }

    void setFitness(std::size_t index, float value)
    {
        assert(index < m_genomes.size());
        m_fitness[index] = value;
    }

    [[nodiscard]] float getFitness(std::size_t index) const
    {
        assert(index < m_fitness.size());
        return m_fitness[index];
    }

    void clear()
    {
        m_genomes.clear();
        m_fitness.clear();
    }

    void addGenome(Genome genome)
    {
        m_genomes.push_back(std::move(genome));
        m_fitness.push_back(0.0f);
    }

    // Dynamic Sizing (Coach Policy)
    void resize(std::size_t newSize) {
        if (m_genomes.size() != newSize) {
            m_genomes.resize(newSize);
            m_fitness.resize(newSize);
            m_cfg.populationSize = newSize;
        }
    }

    // Sigma Control (Annealing)
    void setSigma(float s) { m_mutator.setSigma(s); }
    float sigma() const { return m_mutator.sigma(); }
    /*
    void evolveNextGeneration()
    {
        if (m_genomes.empty())
            return;

        std::vector<std::size_t> indices(m_genomes.size());
        std::iota(indices.begin(), indices.end(), std::size_t{0});

        std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
            return m_fitness[a] > m_fitness[b];
        });

        std::vector<Genome> nextGen;
        nextGen.reserve(m_cfg.populationSize);

        const std::size_t maxElites =
            std::min<std::size_t>(m_cfg.eliteCount, m_cfg.populationSize);

        for (std::size_t i = 0; i < maxElites && i < indices.size(); ++i)
            nextGen.push_back(m_genomes[indices[i]]);

        const std::size_t poolSize = std::max<std::size_t>(1, indices.size() / 2);

        auto &rng = job::crypto::JobRandom::engine();
        std::uniform_int_distribution<std::size_t> dist(0, poolSize - 1);

        while (nextGen.size() < m_cfg.populationSize) {
            const std::size_t idx1 = indices[dist(rng)];
            const std::size_t idx2 = indices[dist(rng)];

            Genome child = m_crossover.cross(m_genomes[idx1], m_genomes[idx2]);
            m_mutator.perturb(child); // let Mutator use its own config
            nextGen.push_back(std::move(child));
        }

        m_genomes  = std::move(nextGen);
        m_fitness.assign(m_genomes.size(), 0.0f);
    }
*/

private:
    PopulationConfig    m_cfg;
    Mutator             m_mutator;
    Crossover           m_crossover;
    std::vector<Genome> m_genomes;  // size = current population
    std::vector<float>  m_fitness;  // same size as m_genomes
};

} // namespace job::ai::evo
