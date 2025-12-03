#pragma once

#include <cmath>
#include <vector>

#include <job_logger.h>
#include <real_type.h>

namespace job::ai::base {
struct GDNParams {
    std::vector<core::real_t>               beta;  // size C
    std::vector<std::vector<core::real_t>>  gamma; // C x C
};

[[nodiscard]] inline std::vector<core::real_t> gdn(const std::vector<core::real_t> &x, const GDNParams &p)
{
    const std::size_t C = x.size();
    if (p.beta.size() != C || p.gamma.size() != C) {
        JOB_LOG_ERROR("[GDN] shape mismatch: x.size()={}, beta.size()={}, gamma.size()={}",
                      C, p.beta.size(), p.gamma.size());
        std::abort();
    }

    std::vector<core::real_t> y(C);
    for (std::size_t i = 0; i < C; ++i) {
        if (p.gamma[i].size() != C) {
            JOB_LOG_ERROR("[GDN] gamma[{}].size()={} but expected {}", i, p.gamma[i].size(), C);
            std::abort();
        }

        core::real_t denom = p.beta[i];
        for (std::size_t j = 0; j < C; ++j) {
            denom += p.gamma[i][j] * x[j] * x[j];
        }

        // If denom somehow goes non-positive, you may get NaNs. Optional guard:
        denom = std::max(denom, core::real_t(1e-12));

        y[i] = x[i] / std::sqrt(denom);
    }

    return y;
}

}
