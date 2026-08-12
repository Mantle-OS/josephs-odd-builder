#include "job_ggml_opt_sgd_params.h"

#include <stdexcept>

#include <real_type.h>

namespace job::ggml {

JobGgmlOptSgdParams::JobGgmlOptSgdParams(float alpha, float wd) :
    m_alpha{alpha},
    m_wd{wd}
{
    if (!isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptSgdParams received invalid parameters"
        };
    }
}

float JobGgmlOptSgdParams::alpha() const noexcept
{
    return m_alpha;
}

void JobGgmlOptSgdParams::setAlpha(float alpha)
{
    if (!core::isSafeFinite(alpha) || alpha <= 0.0f) {
        throw std::invalid_argument{
            "JobGgmlOptSgdParams alpha must be finite and greater than zero"
        };
    }

    m_alpha = alpha;
}

float JobGgmlOptSgdParams::wd() const noexcept
{
    return m_wd;
}

void JobGgmlOptSgdParams::setWd(float wd)
{
    if (!core::isSafeFinite(wd) || wd < 0.0f) {
        throw std::invalid_argument{
            "JobGgmlOptSgdParams weight decay must be finite and greater than or equal to zero"
        };
    }

    m_wd = wd;
}

bool JobGgmlOptSgdParams::isValid() const noexcept
{
    return core::isSafeFinite(m_alpha) && core::isSafeFinite(m_wd) &&
           m_alpha > 0.0f && m_wd >= 0.0f;
}

void JobGgmlOptSgdParams::reset() noexcept
{
    m_alpha = 1.0e-3f;
    m_wd    = 0.0f;
}

} // namespace job::ggml