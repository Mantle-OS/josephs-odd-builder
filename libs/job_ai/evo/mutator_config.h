#pragma once
#include <cstdint>
namespace job::ai::evo {
struct MutatorConfig {
    float       weightSigma{0.02f};       // StdDev for Gaussian noise
    float       weightMutationProb{1.0f}; // Probability that a weight gets mutated (1.0 = all weights)
    float       addNodeProb{0.0f};
    float       addLinkProb{0.0f};
    float       enableLinkProb{0.0f};
    uint64_t    seed{0}; // 0 = Random
};
} // namespace job::ai::evo
