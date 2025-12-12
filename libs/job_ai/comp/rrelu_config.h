#pragma once

#include <job_random.h>

namespace job::ai::comp {

struct RReLUConfig {
    float           lower = 0.125f;
    float           upper = 0.333f;
    bool            training = true;
};

// training
[[nodiscard]] inline float rrelu(float x, const RReLUConfig &cfg) noexcept
{
    if (!cfg.training) {
        const float alpha = (cfg.lower + cfg.upper) / 2.0f;
        return x > 0.0f ? x : alpha * x;
    }

    float alpha = crypto::JobRandom::uniformReal(cfg.lower, cfg.upper);
    return x > 0.0f ? x : alpha * x;
}
} // namespace job::ai::comp

