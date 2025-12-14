#pragma once
#include <string>
#include "view.h"
#include "paramgroup_type.h"
namespace job::ai::layers {
struct ParamGroupConfig {
    std::string         name;   // Human-readable identifier (e.g. "ffn.w1")
    ParamGroupType      type;   // Semantic group: Weights, Bias, GateWeights, etc.
    cords::ViewR        data;   // Non-owning view into a contiguous float buffer
    // add shape functions here ?
};
} // job::ai::layers
