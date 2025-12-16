#pragma once

#include <cstring>
#include <string>
#include <cstdint>
#include <bit>

#include "learn_type.h"
#include "token_type.h"


namespace job::ai::learn {

struct LearnConfig {
    // 8 byte
    const char          *corpus         = nullptr;                      // The World Data
    uint64_t            seed            = 12345;                        // Deterministic seeding (Physics/Mutations)
    // 4 byte
    uint32_t            maxSteps        = 500;                          // How long until we declare victory?
    uint32_t            contextLen      = 12;
    float               targetFitness   = 100.0f;                       // Stop condition (if > 0)
    // 1 Byte
    LearnType           type            = LearnType::XOR;               // What type of learner this is
    uint8_t             initWsMb        = 1;                            // Workspace size for the Inference Runner
    token::TokenType    tokenType       = token::TokenType::Motif;      // The token type that this thing is currently speaking
};

[[nodiscard]] inline constexpr bool punishLearner(float data) noexcept
{
    uint32_t bits = std::bit_cast<uint32_t>(data);
    return (bits & 0x7F800000u) == 0x7F800000u;
}
namespace LearnPresets{
    [[nodiscard]] inline constexpr LearnConfig XORConfig()
    {
        LearnConfig cfg;
        cfg.type = LearnType::XOR;
        cfg.targetFitness = 100.0f;
        return cfg;
    }

    [[nodiscard]] inline constexpr LearnConfig CartPoleConfig(uint32_t steps = 500)
    {
        LearnConfig cfg;
        cfg.type = LearnType::CartPole;
        cfg.maxSteps = steps;
        cfg.targetFitness = (float)steps;
        cfg.targetFitness = 100.0f;
        return cfg;
    }

    [[nodiscard]] inline constexpr LearnConfig BardConfig(const char *corpus = "", token::TokenType tt = token::TokenType::Motif)
    {
        LearnConfig cfg;
        cfg.type = LearnType::Bard;
        cfg.corpus = corpus;
        cfg.tokenType = tt;
        cfg.targetFitness = 98.0f; // Good enough for jazz
        return cfg;
    }
}
}
