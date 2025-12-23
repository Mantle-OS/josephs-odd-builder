#pragma once

#include <cmath>
#include <algorithm>
#include <type_traits>

#include <job_logger.h>

#include "activation_types.h"

namespace job::ai::comp {

// Magic numbers from the Self-Normalizing Neural Networks paper
// inline constexpr float kSeluAlpha        = 1.6732632423543772848170429916717f;
// inline constexpr float kSeluScale        = 1.0507009873554804934193349852946f;
// inline constexpr float kApproxGeluCoeff  = 0.044715f;
// inline constexpr float kSqrt2V           = 1.41421356237309504880f;
// inline constexpr float kInvSqrt2         = 0.70710678118654752440f;                         // 1 / sqrt(2)
// inline constexpr float kSqrt2DivPi       = 0.79788456080286535587989f;                      // sqrt(2 / pi)
// inline constexpr float pi                = 3.141592653589793238462643383279502884L;


[[nodiscard]] inline float sigmoid(float x) noexcept
{
    return 1.0f / (1.0f + std::exp(-x));
}

[[nodiscard]] inline float tanhAct(float x) noexcept
{
    return std::tanh(x);
}

[[nodiscard]] inline float hTanh(float x) noexcept
{
    return std::clamp(x, -1.0f, 1.0f);
}

[[nodiscard]] inline float softplus(float x) noexcept
{
    // x is large (>20), ln(1+e^x) converges to x
    if (x > 20.0f)
        return x;
    return std::log(1.0f + std::exp(x));
}

[[nodiscard]] inline float relu(float x) noexcept
{
    return std::max(0.0f, x);
}

[[nodiscard]] inline float leakyRelu(float x, float alpha = 0.01f) noexcept
{
    return (x > 0.0f) ? x : (alpha * x);
}

[[nodiscard]] inline float prelu(float x, float alpha) noexcept
{
    return (x > 0.0f) ? x : (alpha * x);
}

[[nodiscard]] inline float elu(float x, float alpha = 1.0f) noexcept
{
    return (x > 0.0f) ? x : alpha * (std::exp(x) - 1.0f);
}

[[nodiscard]] inline float selu(float x) noexcept
{
    return kSeluScale * ((x > 0.0f) ? x : kSeluAlpha * (std::exp(x) - 1.0f));
}

[[nodiscard]] inline float relu6(float x) noexcept
{
    return std::min(std::max(0.0f, x), 6.0f);
}

// 0.5 * x * (1 + erf(x / sqrt(2)))
[[nodiscard]] inline float gelu(float x) noexcept
{
    return 0.5f * x * (1.0f + std::erf(x * kInvSqrt2));
}

// Tanh based - Faster, no erf() ?????
[[nodiscard]] inline float approxGelu(float x) noexcept
{
    const float inner = kSqrt2DivPi * (x + kApproxGeluCoeff * x * x * x);
    return 0.5f * x * (1.0f + std::tanh(inner));
}

[[nodiscard]] inline float swish(float x, float beta = 1.0f) noexcept
{
    return x * sigmoid(beta * x);
}

[[nodiscard]] inline float hSwish(float x) noexcept
{
    return x * relu6(x + 3.0f) / 6.0f;
}

[[nodiscard]] inline float mish(float x) noexcept
{
    return x * std::tanh(softplus(x));
}

// 0.5 * x * min(2, max(0, x + 2))
[[nodiscard]] inline float hMish(float x) noexcept
{
    float h = std::max(0.0f, x + 2.0f);
    h = std::min(2.0f, h);
    return 0.5f * x * h;
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
[[nodiscard]] inline float rRelu(float x, float alpha) noexcept
{
    return x > 0.0f ? x : alpha * x;
}

[[nodiscard]] inline float activate(ActivationType type, float x, float alpha = 0.0f)
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

inline void activate_buffer_serial(float *buffer, size_t count, ActivationType type, float alpha = 0.0f)
{
    // If alpha is 0.0f, set sensible defaults for parameterized activations
    const float param = alpha;

    switch (type) {
    case ActivationType::Identity:
        return; // No-op
    case ActivationType::Sigmoid:
        for (size_t i = 0; i < count; ++i) buffer[i] =
                sigmoid(buffer[i]);
        break;
    case ActivationType::Tanh:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = tanhAct(buffer[i]);
        break;
    case ActivationType::HardTanh:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = hTanh(buffer[i]);
        break;
    case ActivationType::ReLU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = relu(buffer[i]);
        break;
    case ActivationType::LeakyReLU: {
        const float a = (param != 0.0f) ? param : 0.01f;
        for (size_t i = 0; i < count; ++i)
            buffer[i] = leakyRelu(buffer[i], a);
        break;
    }
    case ActivationType::PReLU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = prelu(buffer[i], param);
        break;
    case ActivationType::ELU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = elu(buffer[i], param);
        break;
    case ActivationType::SELU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = selu(buffer[i]);
        break;
    case ActivationType::GELU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = gelu(buffer[i]);
        break;
    case ActivationType::AproxGELU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = approxGelu(buffer[i]);
        break;
    case ActivationType::Swish: {
        const float beta = (param != 0.0f) ? param : 1.0f;
        for (size_t i = 0; i < count; ++i)
            buffer[i] = swish(buffer[i], beta);
        break;
    }
    case ActivationType::HSwish:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = hSwish(buffer[i]);
        break;
    case ActivationType::Mish:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = mish(buffer[i]);
        break;
    case ActivationType::HMish:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = hMish(buffer[i]);
        break;
    case ActivationType::Softplus:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = softplus(buffer[i]);
        break;
    case ActivationType::RReLU:
        for (size_t i = 0; i < count; ++i)
            buffer[i] = rRelu(buffer[i], param);
        break;
    default:
        // Fallback to scalar switch if needed, though usually covered
        for (size_t i = 0; i < count; ++i)
            buffer[i] = activate(type, buffer[i], param);
        break;
    }
}
} // namespace job::ai::comp
