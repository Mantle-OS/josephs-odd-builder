#pragma once

#include <vector>
#include <memory>
#include "moe_types.h"
#include "layers/ilayer.h"
#include "cords/view.h"

namespace job::ai::moe {

class IMoE : public layers::ILayer {
public:
    virtual ~IMoE() = default;

    // Expert Management
    // An expert is just an ILayer (usually a Dense/MLP layer).
    // The ID allows the router to map tokens to specific experts.
    virtual void addExpert(int id, std::shared_ptr<layers::ILayer> expert) = 0;

    // Router Configuration
    virtual void setRouterType(RouterType type) = 0;
    virtual void setLoadBalancing(LoadBalanceStrategy strategy) = 0;

    // Routing Logic (Public for introspection/visualization)
    // Returns indices of experts for the given input batch.
    // [Batch, 1] -> [Batch, TopK]
    [[nodiscard]] virtual std::vector<int> route(const cords::ViewR& input) = 0;
};

} // namespace job::ai::moe
