#pragma once

#include <cstdint>

namespace job::ai::evo {

struct MutatorConfig {
    float          weightSigma{0.02f};        // StdDev for Gaussian noise
    float          weightMutationProb{1.0f};  // Probability a weight gets mutated [0,1] (1.0 = all weights)

    float          addNodeProb{0.0f};         // NEAT-style: probability of adding a node [0,1]
    float          addLinkProb{0.0f};         // NEAT-style: probability of adding a connection [0,1]
    float          enableLinkProb{0.0f};      // Probability of re-enabling a disabled link [0,1]

    std::uint64_t  seed{0};                   // 0 = use global/random seed, non-zero = deterministic stream
};

} // namespace job::ai::evo
