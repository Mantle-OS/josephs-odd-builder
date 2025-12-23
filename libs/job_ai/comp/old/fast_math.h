#pragma once
#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;
struct FastMath {
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
        auto zero = SIMD::zero();
        auto neg_x = SIMD::sub(zero, x);
        auto e_neg_x = exp_schraudolph(neg_x);
        auto denom = SIMD::add(one, e_neg_x);
        return SIMD::div(one, denom);
    }

    static inline f32 tanh_fast(f32 x)
    {
        // tanh(x) = (e^2x - 1) / (e^2x + 1)
        auto one = SIMD::set1(1.0f);
        auto two_x = SIMD::add(x, x);
        auto e_2x = exp_schraudolph(two_x);

        auto num = SIMD::sub(e_2x, one);
        auto den = SIMD::add(e_2x, one);
        return SIMD::div(num, den);
    }

};
}
