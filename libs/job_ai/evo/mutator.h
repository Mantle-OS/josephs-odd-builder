#pragma once

#include <random>
#include <vector>
#include <cmath>

#include <job_random.h>

#include "mutator_config.h"
#include "genome.h"

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

    // "Sigma" override allows annealing
    void perturb(Genome &genome, float sigma)
    {
        if (m_cfg.weightMutationProb <= 0.0f)
            return;

        // job::crypto::JobRandom to the ... engine (zero overhead)
        auto &rng = job::crypto::JobRandom::engine();
        // ONCE for perf
        std::normal_distribution<float> dist(0.0f, sigma);

        // fast path (mutate all)
        if (m_cfg.weightMutationProb >= 1.0f) {
            for (float &w : genome.weights)
                w += dist(rng);
        } else {
            std::uniform_real_distribution<float> prob(0.0f, 1.0f);
            for (float& w : genome.weights)
                if (prob(rng) < m_cfg.weightMutationProb)
                    w += dist(rng);
        }
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
