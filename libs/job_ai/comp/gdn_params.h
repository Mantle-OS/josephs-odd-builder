#pragma once

#include <cmath>
#include <vector>

#include <job_logger.h>

namespace job::ai::comp {
struct GDNParams {
    std::vector<float>               beta;  // size C
    std::vector<std::vector<float>>  gamma; // C x C
};

[[nodiscard]] inline std::vector<float> gdn(const std::vector<float> &x, const GDNParams &p)
{
    const std::size_t C = x.size();
    if (p.beta.size() != C || p.gamma.size() != C) {
        JOB_LOG_ERROR("[GDN] shape mismatch: x.size()={}, beta.size()={}, gamma.size()={}",
                      C, p.beta.size(), p.gamma.size());
        std::abort();
    }

    std::vector<float> y(C);
    for (std::size_t i = 0; i < C; ++i) {
        if (p.gamma[i].size() != C) {
            JOB_LOG_ERROR("[GDN] gamma[{}].size()={} but expected {}", i, p.gamma[i].size(), C);
            std::abort();
        }

        float denom = p.beta[i];
        for (std::size_t j = 0; j < C; ++j)
            denom += p.gamma[i][j] * x[j] * x[j];

        // If denom somehow goes non-positive, you may get NaNs. Optional guard:
        denom = std::max(denom, 1e-12f);

        y[i] = x[i] / std::sqrt(denom);
    }

    return y;
}

} // namespace job::ai::comp
