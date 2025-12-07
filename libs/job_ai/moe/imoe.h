#pragma once

#include <vector>
#include <memory>

#include "moe_types.h"

#include "router_plan.h"
#include "router_types.h"

#include "ilayer.h"

#include "view.h"
#include "workspace.h"

namespace job::ai::moe {

class IMoE : public layers::ILayer {
public:
    virtual ~IMoE() = default;
    // Expert Management
    // An expert is just an ILayer (usually a Dense/MLP layer).
    // The ID allows the router to map tokens to specific experts.
    virtual void addExpert(int id, layers::ILayer::Ptr expert) = 0;

    [[nodiscard]] virtual layers::LayerType type() const = 0;
    [[nodiscard]] virtual std::string name() const = 0;

    // Router Configuration
    virtual void setRouterType(router::RouterType type) = 0;
    virtual void setLoadBalancing(router::LoadBalanceStrategy strategy) = 0;
    virtual router::RouterPlan route(threads::ThreadPool &pool, const cords::ViewR &input,
                                     infer::Workspace &ws,
                                     std::vector<float> *maybeGateLogits) = 0;

    // virtual void forward(job::threads::ThreadPool &pool,
    //              const cords::ViewR &input,
    //              infer::Workspace &ws,
    //              cords::ViewR &output) = 0;
};

} // namespace job::ai::moe
