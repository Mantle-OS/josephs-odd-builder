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
    int                         batchSize{0};
    int                         numExperts{0};
    RouterToken                 *tokens = nullptr;
    size_t                      tokenCount{0};

    [[nodiscard]] RouterToken *begin()
    {
        return tokens;
    }
    [[nodiscard]] RouterToken *end()
    {
        return tokens + tokenCount;
    }
    [[nodiscard]] const RouterToken *begin() const
    {
        return tokens;
    }
    [[nodiscard]] const RouterToken *end() const
    {
        return tokens + tokenCount;
    }
};
} // namespace job::ai::router
