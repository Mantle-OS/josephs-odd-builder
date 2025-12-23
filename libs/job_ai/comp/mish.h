#pragma once
#include <cmath>
#include <simd_provider.h>
#include "softplus.h"
#include "tanh.h"

namespace job::ai::comp {
using namespace job::simd;
template<bool T_SMOOTH>
struct FunctorMish {
    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        // x * tanh(softplus(x))
        auto sp = FunctorSoftplus<T_SMOOTH>::apply(x, alpha);
        return SIMD::mul(x, FunctorTann<T_SMOOTH>::apply(sp, alpha));
    }

    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        auto sp = FunctorSoftplus<T_SMOOTH>::apply_naive(x);
        return x * FunctorTann<T_SMOOTH>::apply_naive(sp);
    }
};

template<bool T_SMOOTH>
struct FunctorHardMish {
    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        auto half = SIMD::set1(0.5f);
        auto two  = SIMD::set1(2.0f);

        auto x_plus_2 = SIMD::add(x, two);

        auto h = SIMD::clamp_f32(x_plus_2, 0.0f, 2.0f);

        auto half_x = SIMD::mul(x, half);
        return SIMD::mul(half_x, h);
    }

    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        float h = std::max(0.0f, x + 2.0f);
        h = std::min(2.0f, h);
        return 0.5f * x * h;
    }
};
}
