#pragma once
#include <vector>
#include "view.h"
namespace job::ai::comp {

class NoiseSchedule {
public:
    [[nodiscard]] float addNoise(const cords::ViewR &latent, int timestep, float seed);
    [[nodiscard]] float step(const cords::ViewR &epsPred, int timestep, cords::ViewR &xT);
    // ...
private:
    std::vector<float>                    m_sigmas;
    std::vector<float>                    mAlphaBars;
};
}
