#pragma once
#include <cstdint>
#include <string>

#include "view.h"

namespace job::ai::layers {

enum class ParamRole : std::uint8_t {
    Weights,        // Main linear weights (e.g. W in y = xW + b)
    Bias,           // Bias vectors / offsets added after a linear op
    GateWeights,    // Parameters used by gates / routers (MoE gates, attention gates, etc.)
    ExpertWeights,  // Parameters that belong to MoE experts (per-expert layers)
    RouterState,    // Router-specific state (embeddings, running stats, FSM tables, etc.)
    Other           // Anything that doesn’t fit above: scalars, norms, misc buffers
};

struct ParamView {
    std::string  name;
    ParamRole    role;
    cords::ViewR data;
};

using ParameterGroup = std::vector<ParamView>;

class IParamGroupProvider {
public:
    virtual ~IParamGroupProvider() = default;
    virtual ParameterGroup parameterGroups() = 0;
};

}
