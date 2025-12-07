#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include <job_logger.h>
#include <job_random.h>

#include "crossover_config.h"
#include "genome.h"

namespace job::ai::evo {

class Crossover {
public:
    Crossover() = default;

    explicit Crossover(const CrossoverConfig &cfg) :
        m_cfg(cfg)
    {
    }

    Crossover(const Crossover &) = default;
    Crossover(Crossover &&) = default;
    Crossover &operator=(const Crossover &) = default;
    Crossover &operator=(Crossover &&) = default;

    [[nodiscard]] Genome cross(const Genome &p1, const Genome &p2) const
    {
        if (p1.weights.size() != p2.weights.size()) {
            JOB_LOG_ERROR("[Crossover] Parent weight sizes differ: {} vs {}. "
                          "Returning first parent unchanged.",
                          p1.weights.size(), p2.weights.size());
            return p1;
        }

        Genome child = p1; // start as clone, then mutate weights

        switch (m_cfg.type) {
        case CrossoverType::None:
            // no-op, child stays a clone of p1
            break;

        case CrossoverType::Uniform:
            applyUniform(child, p1, p2);
            break;

        case CrossoverType::Arithmetic:
            applyArithmetic(child, p1, p2);
            break;

        case CrossoverType::OnePoint:
        case CrossoverType::TwoPoint:
        case CrossoverType::Neat:
            JOB_LOG_WARN("[Crossover] Crossover type {} not implemented yet; "
                         "using clone of first parent.",
                         static_cast<int>(m_cfg.type));
            break;
        }

        // New child: make it clearly “unevaluated”
        child.tested           = false;
        child.header.fitness   = 0.0f;
        child.header.uuid      = 0;                  // caller/Coach should assign a real ID
        child.header.parentId  = p1.header.uuid;     // track ancestry (second parent optional for now)

        return child;
    }

    void setConfig(const CrossoverConfig &cfg)
    {
        m_cfg = cfg;
    }

    [[nodiscard]] const CrossoverConfig &config() const
    {
        return m_cfg;
    }

private:
    void applyUniform(Genome &child, const Genome &p1, const Genome &p2) const
    {
        auto &rng = job::crypto::JobRandom::engine();
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        const std::size_t n        = p1.weights.size();
        const float       swapProb = m_cfg.uniformSwapProb;

        // float check is "fast" enough.
        for (std::size_t i = 0; i < n; ++i) {
            const float r = dist(rng);
            if (r < swapProb)
                child.weights[i] = p2.weights[i];
            else
                child.weights[i] = p1.weights[i];
        }
    }

    void applyArithmetic(Genome &child, const Genome &p1, const Genome &p2) const
    {
        // Arithmetic is deterministic given alpha, no RNG needed here.
        const float       a = m_cfg.alpha;
        const float       b = 1.0f - a;
        const std::size_t n = p1.weights.size();

        // This loop vectorizes very well (FMA)
        for (std::size_t i = 0; i < n; ++i)
            child.weights[i] = a * p1.weights[i] + b * p2.weights[i];
    }

    CrossoverConfig m_cfg;
};

} // namespace job::ai::evo
