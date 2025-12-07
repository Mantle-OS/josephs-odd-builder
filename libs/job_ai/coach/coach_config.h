#pragma once
#include <cstddef>
#include <cstdint>

namespace job::ai::coach {

struct ESConfig {
    size_t      populationSize      = 64;       // Matches thread count usually
    float       sigma               = 0.02f;    // Mutation strength (Learning Rate)
    float       decay               = 0.99f;    // Sigma decay per generation (Annealing)
    uint64_t    seed                = 0;        // 0 = Random Device

    [[nodiscard]] inline bool validate(const ESConfig &cfg) noexcept
    {
        if (cfg.populationSize == 0)
            return false;

        if (cfg.sigma <= 0.0f)
            return false;

        if (cfg.decay <= 0.0f || cfg.decay > 1.0f)
            return false;

        return true;
    }

};

namespace CoachPresets {

static constexpr ESConfig kFastTest {
    .populationSize     = 8,
    .sigma              = 0.1f,
    .seed               = 42
};

static constexpr ESConfig kStandard {
    .populationSize     = 64,
    .sigma              = 0.02f,
    .decay              = 0.999f,
    .seed               = 0
};

static constexpr ESConfig kDeepSearch {
    .populationSize     = 1024, // Requires cluster
    .sigma              = 0.05f,
    .decay              = 0.9999f,
    .seed               = 0
};

} // namespace CoachPresets
} // namespace job::ai::coach
