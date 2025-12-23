#pragma once
#include <algorithm>
#include <simd_provider.h>
namespace job::ai::comp {

using namespace job::simd;

template<bool T_SMOOTH>
struct FunctorReLU {
    static inline f32 apply(f32 x,[[maybe_unused]] float alpha = 1.0f)
    {
        return SIMD::max(x, SIMD::zero());
    }

    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        return std::max(0.0f, x);
    }
};

template<bool T_SMOOTH>
struct FunctorLeakyReLU {
    static inline f32 apply(f32 x, float alpha = 1.0f)
    {
        // x > 0 ? x : alpha * x
        auto zero = SIMD::zero();
        auto v_alpha = SIMD::set1(alpha); // Broadcast alpha
        auto mask = SIMD::gt_ps(x, zero);
        auto scaled = SIMD::mul(x, v_alpha);
        return SIMD::blendv(scaled, x, mask);
    }

    static inline float apply_naive(float x, float alpha)
    {
        return x > 0.0f ? x : alpha * x;
    }
};


// PReLU: f(x) = { x if x > 0, alpha * x if x <= 0 }
template<bool T_SMOOTH>
struct FunctorPReLU {
    static inline f32 apply(f32 x, float alpha = 1.0f)
    {
        // Note: Ideally PReLU uses a per-channel alpha (vector),
        // but this signature enforces a scalar broadcast.
        auto zero = SIMD::zero();
        auto v_alpha = SIMD::set1(alpha);     // Broadcast scalar alpha to vector
        auto mask = SIMD::gt_ps(x, zero);     // Mask = (x > 0)

        // Calculate the negative side: alpha * x
        auto scaled = SIMD::mul(x, v_alpha);

        // Blend: If mask is true (x > 0), pick x. Else pick scaled.
        return SIMD::blendv(scaled, x, mask);
    }

    static inline float apply_naive(float x, float alpha)
    {
        return (x > 0.0f) ? x : (alpha * x);
    }
};


// RReLU: f(x) = { x if x > 0, alpha * x if x <= 0 }
template<bool T_SMOOTH>
struct FunctorRReLU {
    static inline f32 apply(f32 x, float alpha = 1.0f)
    {
        // The 'alpha' here is assumed to be the randomized slope drawn
        // for this specific inference/training step.
        auto zero = SIMD::zero();
        auto v_alpha = SIMD::set1(alpha);     // Broadcast scalar alpha
        auto mask = SIMD::gt_ps(x, zero);     // Mask = (x > 0)

        // Calculate the negative slope
        auto scaled = SIMD::mul(x, v_alpha);

        // Blend based on mask
        return SIMD::blendv(scaled, x, mask);
    }

    static inline float apply_naive(float x, float alpha)
    {
        return x > 0.0f ? x : alpha * x;
    }
};

}
