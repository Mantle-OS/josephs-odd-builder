#pragma once

#include <cmath>
#include <random>
#include <vector>
#include <algorithm>

#include "noise_table.h"
// job::core
#include <split_mix64.h>

// job::ai
#include "mutator_config.h"
#include "genome.h"
namespace job::ai::evo {

class Mutator {
public:
    // Mutator()
    // {
    //     // default to non-deterministic random if not explicitly seeded (IE I hate this but it is needed)
    //     std::random_device rd;
    //     m_rng.seed(rd());
    // }
    Mutator() :
        m_rng(0)
    {

    }
    explicit Mutator(const MutatorConfig &cfg) :
        m_cfg(cfg),
        m_rng(cfg.seed) // SplitMix64 init is instant
    {
    }
    Mutator(const Mutator &) = default;
    Mutator(Mutator &&) = default;
    Mutator &operator=(const Mutator &) = default;
    Mutator &operator=(Mutator &&) = default;


    // allows the coach to enforce determinism per thread/genome
    void seed(std::uint64_t s)
    {
        m_rng.seed(s);
        m_cfg.seed = s; // Update config to match reality
    }

    // 3. Annealing Support (Coach calls this via Population)
    void setSigma(float s) { m_cfg.weightSigma = s; }
    float sigma() const { return m_cfg.weightSigma; }


    void perturb(Genome &genome)
    {
        perturb(genome, m_cfg.weightSigma);
    }

    // "sigma" override allows annealing from the coach
    void perturb(Genome &genome, float sigma)
    {
        if (genome.weights.empty())
            return;

        const float mutationProb = std::clamp(m_cfg.weightMutationProb, 0.0f, 1.0f);
        if (mutationProb <= 0.0f)
            return;

        // fast path: dense mutation (standard E.S)
        if (mutationProb >= 1.0f) {
            // std::uniform_int_distribution<uint64_t> dist;
            uint64_t noiseSeed = m_rng.next();
            job::ai::comp::NoiseTable::instance().perturb(
                genome.weights.data(),
                genome.weights.size(),
                noiseSeed,
                sigma
                );
        } else {
            constexpr float kTwoPi = 6.28318530718f;

            for (float &w : genome.weights) {
                // Check probability (0..1)
                if (m_rng.nextFloat() < mutationProb) {

                    // Manual Box-Muller Transform
                    // Generates standard Gaussian noise from two uniform variables.
                    // Formula: sqrt(-2 * ln(u1)) * cos(2 * pi * u2)

                    // u1 must be in (0, 1] to avoid log(0)
                    float u1 = 1.0f - m_rng.nextFloat();
                    float u2 = m_rng.nextFloat();

                    float radius = std::sqrt(-2.0f * std::log(u1));
                    float theta  = kTwoPi * u2;

                    // Optimization: We technically generate 2 gaussians (sin/cos).
                    // We discard the 'sin' one here for simplicity/register pressure,
                    // but it's cheap enough to re-compute.
                    float noise = radius * std::cos(theta);

                    w += noise * sigma;
                }
            }
        }

        // This genome is no longer the same individual – mark it as unevaluated
        genome.tested         = false;
        genome.header.fitness = 0.0f;
    }

    void setConfig(const MutatorConfig &cfg)
    {
        m_cfg = cfg;
        // if config implies a specific seed, honor it immediately
        if (m_cfg.seed != 0)
            seed(m_cfg.seed);
    }

    [[nodiscard]] const MutatorConfig &config() const { return m_cfg; }

private:
    MutatorConfig m_cfg;
    job::core::SplitMix64 m_rng;
};

} // namespace job::ai::evo
