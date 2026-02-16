#pragma once
#include <cmath>
#include <simd_provider.h>

#include "tanh.h"

namespace job::ai::comp {
using namespace job::simd;

inline constexpr float kApproxGeluCoeff  = 0.044715f;
inline constexpr float kSqrt2DivPi       = 0.79788456080286535587989f;                      // sqrt(2 / pi)

// x * sigmoid(1.702 * x) approximation
template<bool T_SMOOTH>
struct FunctorGELU {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha = 1.0f)
    {
        auto c_magic = SIMD::set1(kSqrt2DivPi);
        auto c_cube  = SIMD::set1(kApproxGeluCoeff);
        auto c_half  = SIMD::set1(0.5f);
        auto c_one   = SIMD::set1(1.0f);

        auto x2 = SIMD::mul(x, x);
        auto x3 = SIMD::mul(x2, x);

        // Keep fused order identical to scalar reference below:
        auto term  = SIMD::mul_plus(x3, c_cube, x);     // 0.044715*x^3 + x
        auto inner = SIMD::mul(term, c_magic);          // √(2/π) * (...)
        auto t     = FunctorTann<T_SMOOTH>::apply(inner, 1.0f); // must be tanh

        auto res = SIMD::add(c_one, t);
        res = SIMD::mul(res, x);
        return SIMD::mul(res, c_half);
    }

    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        // Match SIMD operation ordering with fmaf to mirror mul_plus + mul
        float x3    = x * x * x;
        float term  = std::fmaf(kApproxGeluCoeff, x3, x);      // 0.044715*x^3 + x
        float inner = term * kSqrt2DivPi;                      // √(2/π) * (...)
        return 0.5f * x * (1.0f + std::tanh(inner));
    }
};

// Formula: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))
template<bool T_SMOOTH>
struct FunctorAproxGELU {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha = 1.0f)
    {
        auto half = SIMD::set1(0.5f);
        auto one  = SIMD::set1(1.0f);
        auto k_coeff = SIMD::set1(kApproxGeluCoeff);
        auto k_sqrt  = SIMD::set1(kSqrt2DivPi);

        auto x2 = SIMD::mul(x, x);
        auto x3 = SIMD::mul(x2, x);
        // inner_poly = (x^3 * coeff) + x
        auto inner_poly = SIMD::mul_plus(x3, k_coeff, x);

        auto arg = SIMD::mul(inner_poly, k_sqrt);

        auto two_arg = SIMD::add(arg, arg);


        f32 e_2x;
        if constexpr (T_SMOOTH)
            e_2x = SIMD::exp(two_arg);
        else
            e_2x = SIMD::exp_fast(two_arg);

        auto num = SIMD::sub(e_2x, one);
        auto den = SIMD::add(e_2x, one);
        auto tanh_val = SIMD::div(num, den);

        // 4. Final: 0.5 * x * (1 + tanh)
        auto one_plus_tanh = SIMD::add(one, tanh_val);
        auto half_x = SIMD::mul(x, half);

        return SIMD::mul(half_x, one_plus_tanh);
    }

    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        const float inner = kSqrt2DivPi * (x + kApproxGeluCoeff * x * x * x);
        return 0.5f * x * (1.0f + std::tanh(inner));
    }
};

}
