#pragma once

#include <random>
#include <vector>
#include <cmath>

#include <job_random.h>

#include "mutator_config.h"
#include "genome.h"
#include "noise_table.h"

namespace job::ai::evo {

class Mutator {
public:
    Mutator() = default;
    explicit Mutator(const MutatorConfig &cfg) :
        m_cfg(cfg)
    {
    }

    Mutator(const Mutator&) = default;
    Mutator(Mutator&&) = default;
    Mutator &operator=(const Mutator&) = default;
    Mutator &operator=(Mutator&&) = default;

    // "Sigma" override allows annealing from the Coach
    void perturb(Genome& genome, float sigma) {
        if (m_cfg.weightMutationProb <= 0.0f) return;

        // FAST PATH: Dense Mutation (Standard ES)
        // If we are mutating every weight (prob >= 1.0), use the AVX Noise Table.
        if (m_cfg.weightMutationProb >= 1.0f) {
            // 1. Get Entropy for Offset
            uint64_t seed = job::crypto::JobRandom::secureU64();

            // 2. Vectorized Add
            job::ai::comp::NoiseTable::instance().perturb(
                genome.weights.data(),
                genome.weights.size(),
                seed,
                sigma
                );
            return;
        }

        // SLOW PATH: Sparse Mutation (Prob < 1.0)
        // We still use standard RNG for sparse selection
        auto& rng = job::crypto::JobRandom::engine();
        std::normal_distribution<float> dist(0.0f, sigma);
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);

        for (float& w : genome.weights)
            if (prob(rng) < m_cfg.weightMutationProb)
                w += dist(rng);
    }

    void setConfig(const MutatorConfig &cfg)
    {
        m_cfg = cfg;
    }
    const MutatorConfig& config() const
    {
        return m_cfg;
    }

private:
    MutatorConfig m_cfg;
};

} // namespace job::ai::evo
