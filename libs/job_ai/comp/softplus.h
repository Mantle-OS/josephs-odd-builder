#pragma once
#include <cmath>
#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;

template<bool T_SMOOTH>
struct FunctorSoftplus {
    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        // Stability: For x > 20, Softplus(x) approx x.
        // This prevents exp(x) from hitting Infinity.
        auto threshold = SIMD::set1(20.0f);
        auto use_linear_mask = SIMD::gt_ps(x, threshold);

        auto one = SIMD::set1(1.0f);
        f32 e_x;

        if constexpr (T_SMOOTH)
            e_x = SIMD::exp(x);
        else
            e_x = SIMD::exp_fast(x);

        // ln(1 + e^x)
        auto sum = SIMD::add(one, e_x);
        auto res = SIMD::log(sum);

        // Blend: If x > 20 return x, else return calculated log
        return SIMD::blendv(res, x, use_linear_mask);
    }
    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        if (x > 20.0f)
            return x;
        return std::log(1.0f + std::exp(x));
    }
};
}
