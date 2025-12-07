#pragma once
#include "router_types.h"
#include "router_expert.h"

#include <vector>
namespace job::ai::router {


struct RouterConfig {
    RouterType                          type                = RouterType::TopK;              // docme
    LoadBalanceStrategy                 lbStrategy          = LoadBalanceStrategy::None;     // docme
    std::vector<RouterExpertConfig>     experts;                                             // size = numExperts
    int                                 topK                = 1;                             // docme
    float                               hashTemperature     = 1.0f;                          // docme
    float                               spatialRadius       = 1.0f;                          // docme
    bool                                deterministic       = true;                          // docme

};

namespace RouterPresets {

// Classic TopK MoE router.
[[nodiscard]] inline RouterConfig TopK(std::uint32_t numExperts, std::uint32_t topK = 2)
{
    RouterConfig cfg;
    cfg.type  = RouterType::TopK;
    cfg.topK  = (numExperts == 0)
                   ? 0
                   : (topK > numExperts ? numExperts : topK);

    cfg.experts.reserve(numExperts);
    for (std::uint32_t i = 0; i < numExperts; ++i) {
        RouterExpertConfig expert{};
        expert.id      = i;
        expert.adapter = adapters::AdapterType::Dense; // sane default; caller can patch
        cfg.experts.push_back(expert);
    }

    return cfg;
}

// Hash-based router: each token goes to exactly one expert, deterministically.
[[nodiscard]] inline RouterConfig Hash(std::uint32_t numExperts)
{
    RouterConfig cfg;
    cfg.type          = RouterType::Hash;
    cfg.topK          = 1;
    cfg.deterministic = true;

    cfg.experts.reserve(numExperts);
    for (std::uint32_t i = 0; i < numExperts; ++i) {
        RouterExpertConfig expert{};
        expert.id      = i;
        expert.adapter = adapters::AdapterType::Dense; // default again
        cfg.experts.push_back(expert);
    }

    return cfg;
}
}

} // namespace job::ai::router
