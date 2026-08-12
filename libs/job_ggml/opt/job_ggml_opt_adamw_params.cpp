#include "job_ggml_opt_adamw_params.h"

#include <stdexcept>

#include <real_type.h>

namespace job::ggml {

JobGgmlOptAdamWParams::JobGgmlOptAdamWParams(float alpha, float beta1, float beta2, float eps, float wd) :
    m_alpha{alpha},
    m_beta1{beta1},
    m_beta2{beta2},
    m_eps{eps},
    m_wd{wd}
{
    if (!isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams received invalid AdamW parameters"
        };
    }
}

float JobGgmlOptAdamWParams::alpha() const noexcept
{
    return m_alpha;
}

void JobGgmlOptAdamWParams::setAlpha(float alpha)
{
    if (!core::isSafeFinite(alpha) || alpha <= 0.0f) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams alpha must be finite and greater than zero"
        };
    }

    m_alpha = alpha;
}

float JobGgmlOptAdamWParams::beta1() const noexcept
{
    return m_beta1;
}

void JobGgmlOptAdamWParams::setBeta1(float beta1)
{
    if (!core::isSafeFinite(beta1) || beta1 < 0.0f || beta1 >= 1.0f) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams beta1 must be finite and within [0, 1]"
        };
    }

    m_beta1 = beta1;
}

float JobGgmlOptAdamWParams::beta2() const noexcept
{
    return m_beta2;
}

void JobGgmlOptAdamWParams::setBeta2(float beta2)
{
    if (!core::isSafeFinite(beta2) || beta2 < 0.0f || beta2 >= 1.0f) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams beta2 must be finite and within [0, 1]"
        };
    }

    m_beta2 = beta2;
}

float JobGgmlOptAdamWParams::eps() const noexcept
{
    return m_eps;
}

void JobGgmlOptAdamWParams::setEps(float eps)
{
    if (!core::isSafeFinite(eps) || eps <= 0.0f) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams eps must be finite and greater than or equal to zero"
        };
    }

    m_eps = eps;
}

float JobGgmlOptAdamWParams::wd() const noexcept
{
    return m_wd;
}

void JobGgmlOptAdamWParams::setWd(float wd)
{
    if (!core::isSafeFinite(wd) || wd < 0.0f || wd > 1.0f) {
        throw std::invalid_argument{
            "JobGgmlOptAdamWParams wd must be finite and within [0, 1]"
        };
    }

    m_wd = wd;
}

bool JobGgmlOptAdamWParams::isValid() const noexcept
{
    return core::isSafeFinite(m_alpha)  &&
           core::isSafeFinite(m_beta1)  &&
           core::isSafeFinite(m_beta2)  &&
           core::isSafeFinite(m_eps)    &&
           core::isSafeFinite(m_wd)     &&
           m_alpha > 0.0f               &&
           m_beta1 >= 0.0f              &&
           m_beta1 < 1.0f               &&
           m_beta2 >= 0.0f              &&
           m_beta2 < 1.0f               &&
           m_eps > 0.0f                 &&
           m_wd >= 0.0f;
}
void JobGgmlOptAdamWParams::reset() noexcept
{
    m_alpha = 0.001f;
    m_beta1 = 0.9f;
    m_beta2 = 0.999f;
    m_eps   = 1.0e-8f;
    m_wd    = 0.0f;
}

} // namespace job::ggml