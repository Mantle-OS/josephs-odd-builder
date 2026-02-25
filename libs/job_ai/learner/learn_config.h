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
    const char          *corpus         = nullptr;                      // The World Data (bytes)
    uint64_t            seed            = 12345;                        // Deterministic seeding (Physics/Mutations)

    // 4 byte
    uint32_t            corpusLen       = 0;                            // World Data length (bytes) [<= 4GiB]
    float               targetFitness   = 100.0f;                       // Stop condition (if > 0)

    // 2 byte
    uint16_t            maxSteps        = 500;                          // How long until we declare victory? [<= 65535]
    uint16_t            contextLen      = 12;                           // Context length [<= 65535]

    // 1 Byte
    LearnType           type            = LearnType::XOR;               // What type of learner this is [XOR, CartPole, Bard]
    uint8_t             initWsMb        = 1;                            // Workspace size(MB) for the Inference Runner
    token::TokenType    tokenType       = token::TokenType::Motif;      // Token dialect [Byte, Ascii, Char, Motif, BPE, None]
    token::StopMode     tokenStopMode   = token::StopMode::Either;      // Selects whether we stop by maxSteps, targetFitness, or whichever happens first.
};

static_assert(sizeof(LearnConfig) == 32);
static_assert(alignof(LearnConfig) == 8);

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
        cfg.targetFitness = 100.0f;
        return cfg;
    }

    [[nodiscard]] inline constexpr LearnConfig LanguageConfig(const char *corpus = "", token::TokenType tt = token::TokenType::Motif)
    {
        LearnConfig cfg;
        cfg.type = LearnType::Language;
        cfg.corpus = corpus;
        cfg.tokenType = tt;
        cfg.targetFitness = 98.0f; // Good enough for jazz
        return cfg;
    }
}
}
