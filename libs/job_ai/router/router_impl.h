#pragma once

#include <vector>

#include "view.h"

#include "router_configs.h"
#include "router_plan.h"

namespace job::ai::router {


[[nodiscard]] RouterPlan routeTopK(const RouterConfig &cfg, int batch, int experts,
                                   const float *logits, RouterToken *buffer);

[[nodiscard]] RouterPlan routeHash(const RouterConfig &cfg, int batch, int experts,
                                   const cords::ViewR &input, RouterToken *buffer);

[[nodiscard]] RouterPlan routeSpatial(const RouterConfig &cfg, int batch,
                                      const cords::ViewR &input, RouterToken *buffer);

[[nodiscard]] RouterPlan routeState(const RouterConfig &cfg, int batch, int experts,
                                    RouterToken *buffer);

} // namespace job::ai::router
