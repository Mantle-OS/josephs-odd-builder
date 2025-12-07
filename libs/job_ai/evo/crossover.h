#pragma once

#include <random>
#include <vector>
#include <algorithm>

#include <job_random.h>

#include "crossover_config.h"
#include "genome.h"


namespace job::ai::evo {

class Crossover {
public:
    Crossover() = default;

    explicit Crossover(const CrossoverConfig &cfg)
        : m_cfg(cfg)
    {}

    Crossover(const Crossover&) = default;
    Crossover(Crossover&&) = default;
    Crossover& operator=(const Crossover&) = default;
    Crossover& operator=(Crossover&&) = default;

    [[nodiscard]] Genome cross(const Genome &p1, const Genome &p2)
    {
        if (p1.weights.size() != p2.weights.size())
            return p1;


        Genome child = p1;

        switch (m_cfg.type) {
        case CrossoverType::None:
            // Just return the clone
            break;
        case CrossoverType::Uniform:
            applyUniform(child, p1, p2);
            break;
        case CrossoverType::Arithmetic:
            applyArithmetic(child, p1, p2);
            break;
        case CrossoverType::OnePoint:
            // TODO: Implement if needed
            break;
        case CrossoverType::TwoPoint:
            // TODO: Implement if needed
            break;
        case CrossoverType::Neat:
            // TODO: Historical marking logic
            break;
        }

        // FIXME
        // Reset lineage info for the new child
        // (Though usually, the caller/Coach handles ID generation)
        child.header.uuid = 0;
        child.header.parent_id = p1.header.uuid;

        return child;
    }

    void setConfig(const CrossoverConfig& cfg)
    {
        m_cfg = cfg;
    }
    const CrossoverConfig& config() const
    {
        return m_cfg;
    }

private:
    void applyUniform(Genome &child, const Genome &p1, const Genome &p2)
    {
        auto &rng = job::crypto::JobRandom::engine();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        const size_t n = p1.weights.size();
        const float swapProb = m_cfg.uniformSwapProb;

        // float check is "fast" enough.
        for (size_t i = 0; i < n; ++i)
            if (dist(rng) < swapProb)
                child.weights[i] = p2.weights[i];
            else
                child.weights[i] = p1.weights[i];
    }

    void applyArithmetic(Genome &child, const Genome &p1, const Genome &p2)
    {
        // Arithmetic is deterministic given alpha, no RNG needed here.
        const float a = m_cfg.alpha;
        const float b = 1.0f - a;
        const size_t n = p1.weights.size();
        // This loop vectorizes very well (FMA)
        for (size_t i = 0; i < n; ++i)
            child.weights[i] = a * p1.weights[i] + b * p2.weights[i];
    }

    CrossoverConfig m_cfg;
};

} // namespace job::ai::evo
