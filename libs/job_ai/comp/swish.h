#pragma once
#include <cassert>
#include <cmath>

#include <simd_provider.h>

#include "sigmoid.h"
namespace job::ai::comp {
using namespace job::simd;
template<bool T_SMOOTH>
struct FunctorSwish {
    static inline f32 apply(f32 x, float alpha = 1.0f)
    {
        // Swish = x * sigmoid(alpha * x)
        // Note: Standard Swish/SiLU is alpha=1.0
        assert(alpha == 1.0f && "Swish alpha should always be 1.0f ... I think");
        auto v_alpha = SIMD::set1(alpha);
        auto inner = SIMD::mul(x, v_alpha);
        return SIMD::mul(x, FunctorSigmoid<T_SMOOTH>::apply(inner));
    }

    static inline float apply_naive(float x, float alpha)
    {
        return x / (1.0f + std::exp(-(alpha * x)));
    }
};

// HSwish (Hard Swish)
template<bool T_SMOOTH>
struct FunctorHardSwish {
    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        // x * clip(x + 3, 0, 6) / 6
        auto zero = SIMD::zero();
        auto six  = SIMD::set1(6.0f);
        auto three = SIMD::set1(3.0f);

        auto tmp = SIMD::add(x, three);
        tmp = SIMD::max(zero, SIMD::min(six, tmp)); // clip

        // x * tmp * (1/6)
        auto inv_six = SIMD::set1(0.16666667f);
        return SIMD::mul(SIMD::mul(x, tmp), inv_six);
    }

    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        return x * std::min(std::max(0.0f, x + 3.0f), 6.0f) / 6.0f;
    }
};
}
