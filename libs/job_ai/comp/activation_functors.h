#pragma once
#include <cmath>
#include <algorithm>

#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;
struct FastMath {

    // ME: Hey there Johann Von Schraudolph
    // Nicol: my name is Nicol N. Schraudolph
    // ME: okay Johann.... I like your painting's
    //
    // schraudolph's exponential (the bit hack)
    // e^x via integer manipulation of ieee-754 exponent fields.
    static inline f32 exp_schraudolph(f32 x)
    {
        // clamp x to prevent overflow/underflow (-87 to 87)
        auto lo = SIMD::set1(-87.0f);
        auto hi = SIMD::set1(87.0f);
        x = SIMD::max(lo, SIMD::min(hi, x));

        // magic scale: 2^23 / ln(2)
        auto a = SIMD::set1(12102203.16f);

        // bias adjustment: 127 * 2^23 - correction
        auto b = SIMD::set1_i32(1064986823);

        // y = x * (2^23 / ln(2))
        auto product = SIMD::mul(x, a);

        auto i_product = SIMD::cvt_f32_i32(product);

        auto i_result = SIMD::add_i32(i_product, b);

        return SIMD::cast_to_float(i_result);
    }

    // useful if schraudolph's stepwise nature causes quantization noise.
    static inline f32 exp_poly5(f32 x)
    {
        auto c0 = SIMD::set1(1.0f);
        auto c1 = SIMD::set1(1.0f);
        auto c2 = SIMD::set1(0.5f);
        auto c3 = SIMD::set1(0.166666667f);
        auto c4 = SIMD::set1(0.041666667f);
        auto c5 = SIMD::set1(0.008333333f);

        x = SIMD::max(SIMD::set1(-80.0f), SIMD::min(SIMD::set1(80.0f), x));

        // Horner's Scheme
        auto term = SIMD::mul_plus(x, c5, c4);
        term = SIMD::mul_plus(x, term, c3);
        term = SIMD::mul_plus(x, term, c2);
        term = SIMD::mul_plus(x, term, c1);
        term = SIMD::mul_plus(x, term, c0);
        return term;
    }

    static inline f32 sigmoid_fast(f32 x)
    {
        auto one = SIMD::set1(1.0f);
        auto neg_x = SIMD::sub(SIMD::zero(), x);
        auto e_neg_x = exp_schraudolph(neg_x);
        auto denom = SIMD::add(one, e_neg_x);
        return SIMD::div(one, denom);
    }
};

struct FunctorIdentity {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha)
    {
        return x;
    }

    static inline float apply_s(float x, [[maybe_unused]] float alpha)
    {
        return x;
    }
};

struct FunctorReLU {
    static inline f32 apply(f32 x,[[maybe_unused]] float alpha)
    {
        return SIMD::max(x, SIMD::zero());
    }

    static inline float apply_s(float x, [[maybe_unused]] float alpha)
    {
        return std::max(0.0f, x);
    }
};

struct FunctorLeakyReLU {
    static inline f32 apply(f32 x, float alpha)
    {
        // x > 0 ? x : alpha * x
        auto zero = SIMD::zero();
        auto v_alpha = SIMD::set1(alpha); // Broadcast alpha
        auto mask = SIMD::gt_ps(x, zero);
        auto scaled = SIMD::mul(x, v_alpha);
        return SIMD::blendv(scaled, x, mask);
    }

    static inline float apply_s(float x, float alpha)
    {
        return x > 0.0f ? x : alpha * x;
    }
};

struct FunctorSwish {
    static inline f32 apply(f32 x, float alpha)
    {
        // Swish = x * sigmoid(alpha * x)
        // Note: Standard Swish/SiLU is alpha=1.0
        auto v_alpha = SIMD::set1(alpha);
        auto inner = SIMD::mul(x, v_alpha);
        return SIMD::mul(x, FastMath::sigmoid_fast(inner));
    }

    static inline float apply_s(float x, float alpha)
    {
        return x / (1.0f + std::exp(-(alpha * x)));
    }
};

struct FunctorGELU {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha)
    {
        // x * sigmoid(1.702 * x) approximation
        // GELU typically doesn't use alpha, but we keep the signature consistent
        auto coeff = SIMD::set1(1.702f);
        auto inner = SIMD::mul(x, coeff);
        return SIMD::mul(x, FastMath::sigmoid_fast(inner));
    }

    static inline float apply_s(float x, [[maybe_unused]] float alpha)
    {
        // Exact GELU (tanh approximation)
        return 0.5f * x * (1.0f + std::tanh(0.7978845608f * (x + 0.044715f * x * x * x)));
    }
};

}
