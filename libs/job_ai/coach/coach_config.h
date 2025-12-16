#pragma once
#include <cstddef>
#include <cstdint>
#include <learn_config.h>

namespace job::ai::coach {

struct ESConfig {
    learn::LearnConfig envConfig;
    size_t      populationSize  = 64;           // 8 bytes
    uint64_t    coachSeed       = 0;            // 8 bytes (Mutator Seed, 0=Random)
    float       sigma           = 0.02f;        // 4 bytes (Learning Rate)
    float       decay           = 0.99f;        // 4 bytes (Annealing)

    [[nodiscard]] static bool validate(const ESConfig &cfg) noexcept
    {
        if (cfg.populationSize == 0) return false;
        if (cfg.sigma <= 0.0f) return false;
        if (cfg.decay <= 0.0f || cfg.decay > 1.0f) return false;

        // We could also validate the envConfig if needed
        // if (cfg.envConfig.initWsMb == 0) return false;

        return true;
    }
};

// Ensure tight packing
static_assert(sizeof(ESConfig) == 56, "ESConfig has padding holes!");

namespace CoachPresets {

// Quick XOR Test
static constexpr ESConfig kFastTest {
    .envConfig      = learn::LearnPresets::XORConfig(), // Nested Designated Init works in C++20!
    .populationSize = 8,
    .coachSeed      = 42,
    .sigma          = 0.1f,
    .decay          = 0.95f
};

// Standard XOR Training
static constexpr ESConfig kStandard {
    .envConfig      = learn::LearnPresets::XORConfig(),
    .populationSize = 64,
    .coachSeed      = 0,
    .sigma          = 0.05f,
    .decay          = 0.995f
};

// CartPole Physics
static constexpr ESConfig kPhysics {
    .envConfig      = learn::LearnPresets::CartPoleConfig(500),
    .populationSize = 64,
    .coachSeed      = 123,
    .sigma          = 0.1f,     // Physics needs more exploration
    .decay          = 0.99f
};

// The Drunk Bard (Needs defaults, user usually overrides corpus)
static constexpr ESConfig kBard {
    .envConfig      = learn::LearnPresets::BardConfig(),
    .populationSize = 32,       // Smaller pop due to memory/CPU cost
    .coachSeed      = 0,
    .sigma          = 0.10f,    // High volatility for creative writing
    .decay          = 0.999f
};

} // namespace CoachPresets
} // namespace job::ai::coach
