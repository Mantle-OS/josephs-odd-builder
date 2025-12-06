#pragma once
#include "router_types.h"
#include "router_expert.h"

#include <vector>
namespace job::ai::router {


struct RouterConfig {
    RouterType                          type                = RouterType::TopK;
    LoadBalanceStrategy                 lbStrategy          = LoadBalanceStrategy::None;
    std::vector<RouterExpertConfig>     experts;                                    // size = numExperts
    int                                 topK                = 1;
    float                               hashTemperature     = 1.0f;
    float                               spatialRadius       = 1.0f;
    bool                                deterministic       = true;

};

namespace RouterPresets {

// FIXME now that we have the experts ........
inline RouterConfig TopK(int numExperts, int topK = 2) {
    RouterConfig cfg;
    cfg.type = RouterType::TopK;
    cfg.topK = topK;
    (void)numExperts;
    return cfg;
}

inline RouterConfig Hash(int numExperts) {
    RouterConfig cfg;
    cfg.type = RouterType::Hash;
    cfg.topK = 1;
    cfg.deterministic = true;
    (void)numExperts;
    return cfg;
}
}

} // namespace job::ai::router
