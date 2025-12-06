#pragma once
#include "adapter_types.h"
#include <vector>
namespace job::ai::router {
struct RouterToken {
    int                         row{0};         // batch index
    int                         expert{0};      // expert index
    float                       weight{0.0};    // gate weight / prob
    adapters::AdapterType       adapter;        // The type of adapter for this token.
};

struct RouterPlan {
    std::vector<RouterToken>    tokens{{}};     // vector of tokens
    int                         numExperts{0};  // how many experts
    int                         batchSize{0};   // size on the plan
};
} // namespace job::ai::router
