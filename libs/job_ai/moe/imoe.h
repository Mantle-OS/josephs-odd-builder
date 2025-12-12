#pragma once

#include <vector>
#include <memory>

#include "moe_types.h"

#include "router_plan.h"
#include "router_types.h"

#include "abstract_layer.h"

#include "view.h"
#include "workspace.h"

namespace job::ai::moe {

class IMoE {
public:
    virtual ~IMoE() = default;

    virtual void addExpert(uint32_t id, layers::AbstractLayer::Ptr expert) = 0;

    // Router Configuration
    virtual void setRouterType(router::RouterType type) = 0;
    virtual void setLoadBalancing(router::LoadBalanceStrategy strategy) = 0;
    virtual router::RouterPlan route(threads::ThreadPool &pool, const cords::ViewR &input,
                                     infer::Workspace &workspace,
                                     std::vector<float> *maybeGateLogits) = 0;
};

} // namespace job::ai::moe
