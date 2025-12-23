#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;

template<bool T_SMOOTH>
struct FunctorTann {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha = 1.0f)
    {
        // tanh(x) = (e^2x - 1) / (e^2x + 1)
        assert(alpha == 1.0f && "Tanh doesn't use alpha");

        auto one = SIMD::set1(1.0f);
        auto two_x = SIMD::add(x, x);
        f32 e_2x;
        if constexpr (T_SMOOTH)
            e_2x = SIMD::exp_estrin(two_x);
        else
            e_2x = SIMD::exp_schraudolph(two_x);

        auto num = SIMD::sub(e_2x, one);
        auto den = SIMD::add(e_2x, one);
        return SIMD::div(num, den);
    }
    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        return std::tanh(x);
    }
};

template<bool T_SMOOTH>
struct FunctorHardTanh {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha = 1.0f)
    {
        // clamp(x, -1, 1)
        return SIMD::clamp_f32(x, -1.0f, 1.0f);
    }
    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        return std::clamp(x, -1.0f, 1.0f);
    }
};

}

