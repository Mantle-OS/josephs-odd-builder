#pragma once

#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

// We remove job_random.h dependency for the core logic
// to ensure strict local determinism.
#include "mutator_config.h"
#include "genome.h"
#include "noise_table.h"

namespace job::ai::evo {

class Mutator {
public:
    Mutator() {
        // Default to non-deterministic random if not explicitly seeded
        std::random_device rd;
        m_rng.seed(rd());
    }

    explicit Mutator(const MutatorConfig &cfg) :
        m_cfg(cfg)
    {
        if (m_cfg.seed != 0) {
            m_rng.seed(m_cfg.seed);
        } else {
            std::random_device rd;
            m_rng.seed(rd());
        }
    }

    // Default copy/move is fine; copying a Mutator duplicates its RNG state.
    // This is acceptable behavior.
    Mutator(const Mutator &) = default;
    Mutator(Mutator &&) = default;
    Mutator &operator=(const Mutator &) = default;
    Mutator &operator=(Mutator &&) = default;

    // --- THE MISSING LINK ---
    // Allows the Coach to enforce determinism per thread/genome
    void seed(std::uint64_t s)
    {
        m_rng.seed(s);
        m_cfg.seed = s; // Update config to match reality
    }

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
        if (mutationProb >= 1.0f) {
            // Generate a sub-seed for the Noise Table using our deterministic RNG.
            // This ensures that if we seeded m_rng with (GenID + ThreadID),
            // the Noise Table lookup is also deterministic.
            std::uniform_int_distribution<uint64_t> dist;
            uint64_t noiseSeed = dist(m_rng);

            job::ai::comp::NoiseTable::instance().perturb(
                genome.weights.data(),
                genome.weights.size(),
                noiseSeed,
                sigma
                );
        } else {
            // Slow path: sparse mutation (prob < 1.0)
            // Use LOCAL m_rng, not global JobRandom (Fixes Data Race)
            std::normal_distribution<float>     dist(0.0f, sigma);
            std::uniform_real_distribution<float> prob(0.0f, 1.0f);

            for (float &w : genome.weights) {
                if (prob(m_rng) < mutationProb)
                    w += dist(m_rng);
            }
        }

        // This genome is no longer the same individual – mark it as unevaluated
        genome.tested         = false;
        genome.header.fitness = 0.0f;
    }

    void setConfig(const MutatorConfig &cfg)
    {
        m_cfg = cfg;
        // If config implies a specific seed, honor it immediately
        if (m_cfg.seed != 0) {
            seed(m_cfg.seed);
        }
    }

    [[nodiscard]] const MutatorConfig &config() const
    {
        return m_cfg;
    }

private:
    MutatorConfig m_cfg;

    // The Internal Engine.
    // Using 64-bit Mersenne Twister for high-quality, reproducible sequences.
    std::mt19937_64 m_rng;
};

} // namespace job::ai::evo
