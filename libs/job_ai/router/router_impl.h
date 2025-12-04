#pragma once

#include <vector>

#include "view.h"

#include "router_configs.h"
#include "router_plan.h"

namespace job::ai::router {

// Logits: [Batch, NumExperts] (Flat buffer)
[[nodiscard]] RouterPlan routeTopK(
    const RouterConfig &cfg,
    const RouterPlan &basePlan, // Contains batch/expert counts
    const float *logits
    );

[[nodiscard]] RouterPlan routeHash(
    const RouterConfig &cfg,
    const RouterPlan &basePlan,
    const cords::ViewR &input
    );

[[nodiscard]] RouterPlan routeSpatial(
    const RouterConfig &cfg,
    const RouterPlan &basePlan,
    const cords::ViewR &input
    );

[[nodiscard]] RouterPlan routeState(
    const RouterConfig &cfg,
    const RouterPlan &basePlan,
    const cords::ViewR &input
    );

} // namespace job::ai::router
