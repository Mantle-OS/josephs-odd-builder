#pragma once

#include "irouter.h"

namespace job::ai::router {

class TopKRouter : public IRouter {

public:
    TopKRouter()
    {
        m_topK              = 1;
        m_hashTemperature   = 1.0f;
        m_spatialRadius     = 1.0f;
        m_deterministic     = true;
        m_type              = RouterType::TopKRouter;
    }


    void route(int batch, int experts, const cords::ViewR &input, float logits = 0.0f) override
    {

    }

    RouterPlan routePlan(threads::ThreadPool &pool,
                                 const cords::ViewR &input,
                                 std::vector<float> *maybeGateLogits = nullptr) override
    {

    }
private:

};
}


