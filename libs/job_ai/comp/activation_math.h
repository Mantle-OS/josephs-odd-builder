#pragma once

#include <cmath>
#include <algorithm>
#include <type_traits>

#include <real_type.h>
#include "activation_types.h"
#include "job_logger.h"

namespace job::ai::comp {

// Magic numbers from the Self-Normalizing Neural Networks paper
inline constexpr core::real_t kSeluAlpha        = core::real_t(1.6732632423543772848170429916717);
inline constexpr core::real_t kSeluScale        = core::real_t(1.0507009873554804934193349852946);
inline constexpr core::real_t kApproxGeluCoeff  = core::real_t(0.044715);

[[nodiscard]] inline core::real_t sigmoid(core::real_t x) noexcept
{
    return core::real_t(1) / (core::real_t(1) + std::exp(-x));
}

[[nodiscard]] inline core::real_t tanhAct(core::real_t x) noexcept
{
    return std::tanh(x);
}

[[nodiscard]] inline core::real_t hTanh(core::real_t x) noexcept
{
    return std::clamp(x, core::real_t(-1), core::real_t(1));
}

[[nodiscard]] inline core::real_t softplus(core::real_t x) noexcept
{
    // x is large (>20), ln(1+e^x) converges to x
    if (x > core::real_t(20))
        return x;
    return std::log(core::real_t(1) + std::exp(x));
}

[[nodiscard]] inline core::real_t relu(core::real_t x) noexcept
{
    return std::max(core::real_t(0), x);
}

[[nodiscard]] inline core::real_t leakyRelu(core::real_t x, core::real_t alpha = core::real_t(0.01)) noexcept
{
    return (x > core::real_t(0)) ? x : (alpha * x);
}

[[nodiscard]] inline core::real_t prelu(core::real_t x, core::real_t alpha) noexcept
{
    return (x > core::real_t(0)) ? x : (alpha * x);
}

[[nodiscard]] inline core::real_t elu(core::real_t x, core::real_t alpha = core::real_t(1.0)) noexcept
{
    return (x > core::real_t(0)) ? x : alpha * (std::exp(x) - core::real_t(1));
}

[[nodiscard]] inline core::real_t selu(core::real_t x) noexcept
{
    return kSeluScale * ((x > core::real_t(0)) ? x : kSeluAlpha * (std::exp(x) - core::real_t(1)));
}

[[nodiscard]] inline core::real_t relu6(core::real_t x) noexcept
{
    return std::min(std::max(core::real_t(0), x), core::real_t(6));
}

// 0.5 * x * (1 + erf(x / sqrt(2)))
[[nodiscard]] inline core::real_t gelu(core::real_t x) noexcept {
    return core::real_t(0.5) * x * (core::real_t(1) + std::erf(x * core::kInvSqrt2));
}

// Tanh based - Faster, no erf() ?????
[[nodiscard]] inline core::real_t approxGelu(core::real_t x) noexcept
{
    const core::real_t inner = core::kSqrt2DivPi * (x + kApproxGeluCoeff * x * x * x);
    return core::real_t(0.5) * x * (core::real_t(1) + std::tanh(inner));
}

[[nodiscard]] inline core::real_t swish(core::real_t x, core::real_t beta = core::real_t(1)) noexcept
{
    return x * sigmoid(beta * x);
}

[[nodiscard]] inline core::real_t hSwish(core::real_t x) noexcept
{
    return x * relu6(x + core::real_t(3)) / core::real_t(6);
}

[[nodiscard]] inline core::real_t mish(core::real_t x) noexcept
{
    return x * std::tanh(softplus(x));
}

// 0.5 * x * min(2, max(0, x + 2))
[[nodiscard]] inline core::real_t hMish(core::real_t x) noexcept
{
    core::real_t h = std::max(core::real_t(0), x + core::real_t(2));
    h = std::min(core::real_t(2), h);
    return core::real_t(0.5) * x * h;
}

template <class It>
[[nodiscard]] inline auto maxout(It first, It last)-> typename std::iterator_traits<It>::value_type
{
    using T = typename std::iterator_traits<It>::value_type;
    if (first == last)
        return T(0);

    return *std::max_element(first, last);
}

// alpha is already chosen (random or mean) see rrelu_config.h
[[nodiscard]] inline core::real_t rRelu(core::real_t x, core::real_t alpha) noexcept
{
    return x > core::real_t(0) ? x : alpha * x;
}

[[nodiscard]] inline core::real_t activate(ActivationType type, core::real_t x, core::real_t alpha = 0)
{

    switch (type) {
    case ActivationType::Identity:
        return x;
    case ActivationType::Sigmoid:
        return sigmoid(x);
    case ActivationType::Tanh:
        return tanhAct(x);
    case ActivationType::HardTanh:
        return hTanh(x);
    case ActivationType::ReLU:
        return relu(x);
    case ActivationType::LeakyReLU:
        return leakyRelu(x, alpha);
    case ActivationType::PReLU:
        return prelu(x, alpha);
    case ActivationType::ELU:
        return elu(x, alpha);
    case ActivationType::SELU:
        return selu(x);
    case ActivationType::GELU:
        return gelu(x);
    case ActivationType::AproxGELU:
        return approxGelu(x);
    case ActivationType::Swish:
        return swish(x);
    case ActivationType::HSwish:
        return hSwish(x);
    case ActivationType::Mish:
        return mish(x);
    case ActivationType::HMish:
        return hMish(x);
    case ActivationType::Softplus:
        return softplus(x);
    case ActivationType::RReLU:
        return rRelu(x, alpha);
    case ActivationType::Maxout:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::maxout(range) on its own");
        break;
    case ActivationType::GDN:
        JOB_LOG_ERROR("[ActivationMath]: Please use job::ai::comp::rrelu(x, config)"); // I guess we could call this as well now
        break;
    default:
        return x;
    }
    return x;
}

} // namespace job::ai::comp
