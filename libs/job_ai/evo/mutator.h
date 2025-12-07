#pragma once

#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

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

    Mutator(const Mutator &) = default;
    Mutator(Mutator &&) = default;
    Mutator &operator=(const Mutator &) = default;
    Mutator &operator=(Mutator &&) = default;

    // Use configured sigma
    void perturb(Genome &genome)
    {
        perturb(genome, m_cfg.weightSigma);
    }

    // "Sigma" override allows annealing from the Coach
    void perturb(Genome &genome, float sigma)
    {
        if (genome.weights.empty())
            return;

        // Clamp prob into [0,1]
        const float mutationProb = std::clamp(m_cfg.weightMutationProb, 0.0f, 1.0f);
        if (mutationProb <= 0.0f)
            return;

        // Fast path: dense mutation (standard ES)
        // Mutating every weight (prob ~ 1.0), use the AVX Noise Table.
        if (mutationProb >= 1.0f) {
            // Entropy for offset; coach can control via MutatorConfig::seed later if needed.
            std::uint64_t seed = (m_cfg.seed == 0)
                                     ? job::crypto::JobRandom::secureU64()
                                     : m_cfg.seed; // simple policy for now

            job::ai::comp::NoiseTable::instance().perturb(
                genome.weights.data(),
                genome.weights.size(),
                seed,
                sigma
                );
        } else {
            // Slow path: sparse mutation (prob < 1.0)
            auto &rng = job::crypto::JobRandom::engine();
            std::normal_distribution<float>     dist(0.0f, sigma);
            std::uniform_real_distribution<float> prob(0.0f, 1.0f);

            for (float &w : genome.weights) {
                if (prob(rng) < mutationProb)
                    w += dist(rng);
            }
        }

        // This genome is no longer the same individual – mark it as unevaluated
        genome.tested         = false;
        genome.header.fitness = 0.0f;
        // uuid / parentId are still Coach territory; mutation alone doesn't define new lineage
    }

    void setConfig(const MutatorConfig &cfg)
    {
        m_cfg = cfg;
    }

    [[nodiscard]] const MutatorConfig &config() const
    {
        return m_cfg;
    }

private:
    MutatorConfig m_cfg;
};

} // namespace job::ai::evo
