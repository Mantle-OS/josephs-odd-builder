#pragma once
#include <cstdint>

namespace job::ai::coach {

enum class CoachType : uint8_t {
    Genetic     = 0,    // Standard GA (Crossover + Mutation)
    ES          = 1,    // Evolution Strategy (Gaussian Perturbation, no crossover)
    CMA_ES      = 2,    // Covariance Matrix Adaptation (Advanced)
    // ADD_MORE
};

enum class OptimizationMode : uint8_t {
    Maximize    = 0,    // Higher fitness is better (Standard)
    Minimize    = 1     // Lower fitness is better (Loss function)
};


[[nodiscard]] inline const char *coachTypeName(CoachType type) noexcept
{
    switch (type) {
    case CoachType::Genetic:
        return "Genetic";
    case CoachType::ES:
        return "ES";
    case CoachType::CMA_ES:
        return "CMA_ES";
    }
    return "UnknownCoachType";
}

[[nodiscard]] inline const char *optimizationModeName(OptimizationMode m) noexcept
{
    switch (m) {
    case OptimizationMode::Maximize:
        return "Maximize";
    case OptimizationMode::Minimize:
        return "Minimize";
    }
    return "UnknownOptimizationMode";
}

} // namespace job::ai::coach
