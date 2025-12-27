#pragma once
#include <job_stencil_boundary.h>
namespace job::ai::adapters {
struct StencilConfig {
    int                         steps           = 1;                                // Simulation steps
    float                       diffusionRate   = 0.1f;                             // Rate
    science::BoundaryMode       boundary        = science::BoundaryMode::Wrap;
};

}
