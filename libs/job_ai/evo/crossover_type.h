#pragma once

#include <cstdint>

namespace job::ai::evo {

enum class CrossoverType : std::uint8_t {
    None       = 0, // no crossover, pure cloning/mutation
    OnePoint   = 1, // single cut point in genome
    TwoPoint   = 2, // two cuts, swap middle segment
    Uniform    = 3, // per-gene mixing with probability
    Arithmetic = 4, // weighted average of parents (real-valued genes)
    Neat       = 5  // NEAT-style topology-aware crossover
};

} // namespace job::ai::evo
