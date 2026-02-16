#pragma once

#include "simd_provider.h"
namespace job::simd {

// EXP Schraudolph "Fast Math/BitHack"
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

// Uses Estrin's Scheme (Degree 6) for maximum Instruction Level Parallelism
static inline f32 exp_estrin(f32 x)
{
    //    exp(88) overflows float, exp(-88) underflows to zero
    auto lo = SIMD::set1(-87.5f);
    auto hi = SIMD::set1(87.5f);
    x = SIMD::max(lo, SIMD::min(hi, x));

    auto log2_e = SIMD::set1(1.4426950408f); // 1/ln(2)
    auto k_f = SIMD::round<RoundingMode::Nearest>(SIMD::mul(x, log2_e));
    auto k = SIMD::cvt_f32_i32(k_f);

    // r = x - k * ln(2)
    // We use extended precision (Hi/Lo) to avoid precision loss when x is large
    auto ln2_hi = SIMD::set1(0.6931471806f);
    auto ln2_lo = SIMD::set1(1.4286068203e-6f);

    auto r = SIMD::mul_plus(k_f, SIMD::set1(-1.0f) * ln2_hi, x); // r = x - k*ln2_hi
    r = SIMD::mul_plus(k_f, SIMD::set1(-1.0f) * ln2_lo, r);      // r = r - k*ln2_lo

    auto r2 = SIMD::mul(r, r);
    auto r4 = SIMD::mul(r2, r2);

    // Layer 1: Independent FMAs
    // P01 = (1 * r) + 1
    auto p01 = SIMD::mul_plus(SIMD::set1(1.0f), r, SIMD::set1(1.0f));

    // P23 = (1/6 * r) + 1/2
    auto p23 = SIMD::mul_plus(SIMD::set1(0.166666667f), r, SIMD::set1(0.5f));

    // P45 = (1/120 * r) + 1/24
    auto p45 = SIMD::mul_plus(SIMD::set1(0.008333333f), r, SIMD::set1(0.041666667f));

    // Layer 2: Combine
    // P03 = P23 * r^2 + P01
    auto p03 = SIMD::mul_plus(p23, r2, p01);

    // P46 = (1/720) * r^2 + P45
    auto p46 = SIMD::mul_plus(SIMD::set1(0.001388889f), r2, p45);

    // Layer 3: Final Poly
    // Poly = P46 * r^4 + P03
    auto poly = SIMD::mul_plus(p46, r4, p03);

    // 4. Reconstruction: Result = poly * 2^k
    //    We add 'k' directly to the exponent bits of the float.
    auto exponent_shift = SIMD::slli_epi32(k, 23);
    auto res_int = SIMD::cast_to_int(poly);
    auto final_int = SIMD::add_i32(res_int, exponent_shift);

    return SIMD::cast_to_float(final_int);
}


// FIXME not tested on NEON or 512
static inline f32 avx_log(f32 x)
{

    auto one  = SIMD::set1(1.0f);
    auto zero = SIMD::set1(0.0f);
    auto half = SIMD::set1(0.5f);

    // x <= 0
    auto invalid_mask = SIMD::le_ps(x, zero);

    // x = m * 2^k
    auto x_int = SIMD::cast_to_int(x);

    // k = (exponent_bits >> 23) - 127
    // !!Note!!: Using raw intrinsic for shift because wrapper might be templated in some implementations ?
    auto k_raw = SIMD::sub_i32(SIMD::srli_epi32(x_int, 23), SIMD::set1_i32(0x7F) );
    auto k = SIMD::cvt_i32_f32(k_raw);

    // m = (x & 0x7FFFFF) | 0x3F800000 (Force exponent to 1.0f)
    auto m_mask     = SIMD::set1_i32(0x7FFFFF);
    auto one_bits   = SIMD::set1_i32(0x3F800000);
    auto m_int      = SIMD::or_si(SIMD::and_si(x_int, m_mask), one_bits );
    auto m          = SIMD::cast_to_float(m_int); // m is now in [1.0, 2.0)

    // If m > sqrt(2) -> shift: m = m * 0.5, k = k + 1
    auto sqrt2 = SIMD::set1(1.41421356f);

    // 1.0 where m > sqrt(2)
    auto mask_scale = SIMD::gt_ps(m, sqrt2);

    // m = blend(m, m * 0.5, mask_scale)
    auto m_scaled = SIMD::mul(m, half);
    m = SIMD::blendv(m, m_scaled, mask_scale);

    // k = k + (mask_scale ? 1.0 : 0.0)
    auto k_correction = SIMD::and_ps(mask_scale, one);
    k = SIMD::add(k, k_correction);

    // polynomial: z = m - 1
    auto z = SIMD::sub(m, one);

    // Estrin's scheme (degree 8) minimax polynomial for reduced range
    auto C0 = SIMD::set1(-0.5f);
    auto C1 = SIMD::set1( 0.3333333134f);
    auto C2 = SIMD::set1(-0.2500000000f);
    auto C3 = SIMD::set1( 0.2000000030f);
    auto C4 = SIMD::set1(-0.1666666667f);
    auto C5 = SIMD::set1( 0.1428571492f);
    auto C6 = SIMD::set1(-0.1250000000f);
    auto C7 = SIMD::set1( 0.1111111119f);

    auto z2 = SIMD::mul(z, z);
    auto z4 = SIMD::mul(z2, z2);

    auto p01 = SIMD::mul_plus(C1, z, C0);
    auto p23 = SIMD::mul_plus(C3, z, C2);
    auto p45 = SIMD::mul_plus(C5, z, C4);
    auto p67 = SIMD::mul_plus(C7, z, C6);

    auto p03 = SIMD::mul_plus(p23, z2, p01);
    auto p47 = SIMD::mul_plus(p67, z2, p45);

    auto poly = SIMD::mul_plus(p47, z4, p03);

    // final reconstruction: log(x) = k*ln(2) + z*(1 + z*poly)
    auto ln2_hi = SIMD::set1(0.6931471806f);

    auto r = SIMD::mul_plus(poly, z, one);
    auto poly_term = SIMD::mul(r, z);

    auto res = SIMD::mul_plus(k, ln2_hi, poly_term);

    // handle invalid inputs (fast-math strikes again)
    return SIMD::blendv(res, SIMD::qnan(), invalid_mask);
}



__attribute__((always_inline))
inline float hsum(f32 x) {
    alignas(sizeof(float) * SIMD::width()) float tmp[SIMD::width()];
    SIMD::mov(tmp, x);
    float s = 0.0f;
    for (int i = 0; i < SIMD::width(); ++i)
        s += tmp[i];
    return s;
}



}
