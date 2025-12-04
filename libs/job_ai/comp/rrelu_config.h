#pragma once

#include <real_type.h>
#include <job_random.h>

namespace job::ai::comp {

struct RReLUConfig {
    core::real_t    lower = core::real_t(0.125);
    core::real_t    upper = core::real_t(0.333);
    bool            training = true;
};

// training
[[nodiscard]] inline core::real_t rrelu(core::real_t x, const RReLUConfig &cfg) noexcept
{
    if (!cfg.training) {
        const core::real_t alpha = (cfg.lower + cfg.upper) / core::real_t(2);
        return x > core::real_t(0) ? x : alpha * x;
    }

    core::real_t alpha = crypto::JobRandom::uniformReal(cfg.lower, cfg.upper);
    return x > core::real_t(0) ? x : alpha * x;
}
} // namespace job::ai::comp

