#pragma once

#include <cstdint>

namespace job::ai::evo {

struct MutatorConfig {
    float          weightSigma{0.02f};        // StdDev for Gaussian noise
    float          weightMutationProb{1.0f};  // Probability a weight gets mutated [0,1] (1.0 = all weights)
    std::uint64_t  seed{0};                   // 0 = use global/random seed, non-zero = deterministic stream
};

} // namespace job::ai::evo
