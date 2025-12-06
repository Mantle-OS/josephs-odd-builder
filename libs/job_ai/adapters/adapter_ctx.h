#pragma once
#include "adapter_types.h"
namespace job::ai::adapters {

// !! NOTE !! embedDim, timestep, etc. Adapters can ignore fields they don't care about.
struct AdapterCtx {
    Precision   precision{Precision::Float32};
    float       dt{0.0f};                               // for Verlet / dynamics
    int         embedDim{0};                            // for attention
    int         headDim{0};                             // optional
    // TODO: add padding/mask info later if needed
};

}
