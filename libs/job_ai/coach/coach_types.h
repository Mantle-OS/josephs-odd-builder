#pragma once
#include <cstdint>

namespace job::ai::coach {

enum class CoachType : uint8_t {
    Genetic = 0,    // Standard GA (Crossover + Mutation)
    ES,             // Evolution Strategy (Gaussian Perturbation, no crossover)
    CMA_ES,         // Covariance Matrix Adaptation (Advanced)
    // ADD_MORE
};

enum class OptimizationMode : uint8_t {
    Maximize = 0,   // Higher fitness is better (Standard)
    Minimize        // Lower fitness is better (Loss function)
};

} // namespace job::ai::coach
