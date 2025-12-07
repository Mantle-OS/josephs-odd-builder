#pragma once
#include <cmath>
#include <algorithm>

#include "simd_provider.h"

namespace job::ai::comp {

struct FastMath {

    // ME: Hey there Johann von Schraudolph
    // Nicol: My name is  Nicol N. Schraudolph
    // ME: Okay Johann.... I like your painting's
    //
    // Schraudolph's Exponential (The Bit Hack)
    // e^x via integer manipulation of IEEE-754 exponent fields.
    static inline f32 exp_schraudolph(f32 x)
    {
        // 1. Clamp x to prevent overflow/underflow (-87 to 87)
        auto lo = SIMD::set1(-87.0f);
        auto hi = SIMD::set1(87.0f);
        x = SIMD::max(lo, SIMD::min(hi, x));

        // Magic scale: 2^23 / ln(2)
        auto a = SIMD::set1(12102203.16f);

        // Bias adjustment: 127 * 2^23 - correction
        auto b = SIMD::set1_i32(1064986823);

        // y = x * (2^23 / ln(2))
        auto product = SIMD::mul(x, a);

        auto i_product = SIMD::cvt_f32_i32(product);

        auto i_result = SIMD::add_i32(i_product, b);

        return SIMD::cast_to_float(i_result);
    }

    // Useful if Schraudolph's stepwise nature causes quantization noise.
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

        // Choose your weapon: Schraudolph or Poly5
        auto e_neg_x = exp_schraudolph(neg_x);

        auto denom = SIMD::add(one, e_neg_x);
        return SIMD::div(one, denom);
    }
};

struct FunctorIdentity {
    static inline f32 apply(f32 x)
    {
        return x;
    }
    static inline float apply_s(float x)
    {
        return x;
    }
};

struct FunctorReLU {
    static inline f32 apply(f32 x) {
        return SIMD::max(x, SIMD::zero());
    }
    static inline float apply_s(float x)
    {
        return std::max(0.0f, x);
    }
};

struct FunctorLeakyReLU {
    static inline f32 apply(f32 x) {
        // x > 0 ? x : 0.01x
        auto zero = SIMD::zero();
        auto alpha = SIMD::set1(0.01f);
        auto mask = SIMD::gt_ps(x, zero);
        auto scaled = SIMD::mul(x, alpha);
        return SIMD::blendv(scaled, x, mask);
    }
    static inline float apply_s(float x)
    {
        return x > 0.0f ? x : 0.01f * x;
    }
};

struct FunctorSwish {
    static inline f32 apply(f32 x) {
        // Swish = x * sigmoid(x)
        return SIMD::mul(x, FastMath::sigmoid_fast(x));
    }
    static inline float apply_s(float x)
    {
        return x / (1.0f + std::exp(-x));
    }
};

struct FunctorGELU {
    static inline f32 apply(f32 x) {
        // x * sigmoid(1.702 * x) approximation
        auto coeff = SIMD::set1(1.702f);
        auto inner = SIMD::mul(x, coeff);
        return SIMD::mul(x, FastMath::sigmoid_fast(inner));
    }
    static inline float apply_s(float x)
    {
        return 0.5f * x * (1.0f + std::tanh(0.7978845608f * (x + 0.044715f * x * x * x)));
    }
};

} // namespace
