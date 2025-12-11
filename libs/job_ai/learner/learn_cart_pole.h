#pragma once
#include <cmath>
#include <string>
#include "learn_concept.h"
#include "view.h"
#include "runner.h"

namespace job::ai::job::ai::learn {

class CartPoleLearn {
public:
    static constexpr float kGravity         = 9.8f;
    static constexpr float kMassCart        = 1.0f;
    static constexpr float kMassPole        = 0.1f;
    static constexpr float kTotalMass       = (kMassCart + kMassPole);
    static constexpr float kLength          = 0.5f; // Half-length
    static constexpr float kPoleMassLength  = (kMassPole * kLength);
    static constexpr float kForceMag        = 10.0f;
    static constexpr float kTau             = 0.02f; // Time step (20ms)
    static constexpr int   kMaxSteps        = 500; // 10 seconds of balance

    [[nodiscard]] std::string name() const
    {
        return "CartPole-V1";
    }

    [[nodiscard]] float learn(const evo::Genome &genome) const
    {
        infer::Runner runner(genome, nullptr, 0);

        float x             = 0.0f;
        float x_dot         = 0.0f;
        // Upright (0 rads)
        float theta         = 0.0f;
        float theta_dot     = 0.0f;

        int steps = 0;

        for (; steps < kMaxSteps; ++steps) {
            float inputs[4] = {x, x_dot, theta, theta_dot};
            cords::ViewR inputView(inputs, {1, 4});

            // ask brain
            auto output = runner.run(inputView);
            float actionVal = output.data()[0];

            // apply action (bang-bang control)
            float force = (actionVal > 0.0f) ? kForceMag : -kForceMag;

            // euler integration
            float costheta = std::cos(theta);
            float sintheta = std::sin(theta);

            float temp      = (force + kPoleMassLength * theta_dot * theta_dot * sintheta) / kTotalMass;
            float thetaacc  = (kGravity * sintheta - costheta * temp) / (kLength * (4.0f/3.0f - kMassPole * costheta * costheta / kTotalMass));
            float xacc      = temp - kPoleMassLength * thetaacc * costheta / kTotalMass;

            x += x_dot * kTau;
            x_dot += xacc * kTau;
            theta += theta_dot * kTau;
            theta_dot += thetaacc * kTau;

            // failure conditions:
            // angle > 12 degrees (0.209 rad) or cart out of bounds (+- 2.4 units)
            if (x < -2.4f || x > 2.4f || theta < -0.2094f || theta > 0.2094f)
                break;
        }

        // Fitness: Normalize [0.0, 1.0]
        return static_cast<float>(steps) / static_cast<float>(kMaxSteps);
    }
};

} // namespace
