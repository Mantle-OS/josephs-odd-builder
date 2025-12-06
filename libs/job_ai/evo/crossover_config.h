#pragma once
#include "crossover_type.h"
namespace job::ai::evo {
struct CrossoverConfig {
    CrossoverType   type{CrossoverType::Uniform};
    float           uniformSwapProb{0.5f};              // For Uniform: Chance to take from Parent B
    float           alpha{0.5f};                        // For Arithmetic: child = a*P1 + (1-a)*P2
};

} // namespace job::ai::evo
