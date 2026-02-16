#pragma once
#include <cmath>
#include <simd_provider.h>
namespace job::ai::comp {
using namespace job::simd;

// ELU: f(x) = { x if x > 0, alpha * (exp(x) - 1) if x <= 0 }
template<bool T_SMOOTH>
struct FunctorELU {

    static inline f32 apply(f32 x, float alpha = 1.0f)
    {
        auto zero = SIMD::zero();
        auto one  = SIMD::set1(1.0f);
        auto v_alpha = SIMD::set1(alpha);
        auto mask = SIMD::gt_ps(x, zero); // Mask = (x > 0)

        // Calculate Negative branch: alpha * (exp(x) - 1)
        // Note: Using SIMD::exp from your provider
        [[maybe_unused]] f32 e_x ;
        if constexpr (T_SMOOTH)
            e_x = SIMD::exp(x);
        else
            e_x = SIMD::exp_fast(x);

        auto sub = SIMD::sub(e_x, one);   // (exp(x) - 1)
        auto neg_part = SIMD::mul(sub, v_alpha);

        // Blend: If x > 0 return x, else return neg_part
        return SIMD::blendv(neg_part, x, mask);
    }

    static inline float apply_naive(float x, float alpha)
    {
        return x > 0.0f ? x : alpha * (std::exp(x) - 1.0f);
    }
};



// SELU: f(x) = scale * { x if x > 0, kSeluAlpha * (exp(x) - 1) if x <= 0 }
inline constexpr float kSeluAlpha        = 1.6732632423543772848170429916717f;
inline constexpr float kSeluScale        = 1.0507009873554804934193349852946f;
template<bool T_SMOOTH>
struct FunctorSELU {
    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        // Note: We ignore the passed 'alpha' argument to strictly adhere
        // to the SELU paper's fixed point constants.

        auto zero = SIMD::zero();
        auto one  = SIMD::set1(1.0f);
        auto v_scale = SIMD::set1(kSeluScale);
        auto v_alpha = SIMD::set1(kSeluAlpha); // Use constant, not arg
        auto mask = SIMD::gt_ps(x, zero);      // Mask = (x > 0)

        // Calculate inner negative branch: kSeluAlpha * (exp(x) - 1)
        f32 e_x;
        if constexpr (T_SMOOTH)
            e_x = SIMD::exp(x);
        else
            e_x = SIMD::exp_fast(x);

        auto sub = SIMD::sub(e_x, one);
        auto neg_inner = SIMD::mul(sub, v_alpha);

        // Blend the inner parts first: (x > 0 ? x : neg_inner)
        auto mixed = SIMD::blendv(neg_inner, x, mask);

        // Scale the entire result
        return SIMD::mul(mixed, v_scale);
    }

    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        return kSeluScale * ((x > 0.0f) ? x : kSeluAlpha * (std::exp(x) - 1.0f));
    }
};

}
