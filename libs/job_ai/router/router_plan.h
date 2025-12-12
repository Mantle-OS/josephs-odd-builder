#pragma once
#include <cstddef>
#include "adapter_types.h"
namespace job::ai::router {
struct RouterToken {
    int                         row{0};                                     // batch index
    int                         expert{0};                                  // expert index
    float                       weight{0.0};                                // gate weight / prob
    adapters::AdapterType       adapter{adapters::AdapterType::None};       // The type of adapter for this token.
};

struct RouterPlan {
    int                         batchSize{0};                               // number of rows in the batch
    int                         numExperts{0};                              // total experts available
    RouterToken                 *tokens = nullptr;                          // non-owning pointer to routing table
    std::size_t                 tokenCount{0};                              // total entries in tokens

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
