#pragma once
#include <cstdint>
namespace job::ai::layers {
enum class ParamGroupType : std::uint8_t {
    Weights         = 0,    // Main linear weights (e.g. W in y = xW + b)
    Bias            = 1,    // Bias vectors / offsets added after a linear op
    GateWeights     = 2,    // Parameters used by gates / routers (MoE gates, attention gates, etc.)
    ExpertWeights   = 3,    // Parameters that belong to MoE experts (per-expert layers)
    RouterState     = 4,    // Router-specific state (embeddings, running stats, FSM tables, etc.)
    Other           = 5     // Anything that doesn’t fit above: scalars, norms, misc buffers
};
}
