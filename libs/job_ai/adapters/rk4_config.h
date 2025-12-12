#pragma once
#include "verlet_config.h"
namespace job::ai::adapters {
struct Rk4Config {
    float   dt = 0.01f;                             // Time step
    int     steps = 1;                              // Number of RK4 steps per forward pass
    int     dim_mapping[7] = {0, 1, 2, 3, 4, 5, 6}; // Pos(3), Vel(3), Mass(1)
};
} // namespace job::ai::adapters
