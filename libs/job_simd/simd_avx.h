#pragma once

#include <immintrin.h>
#include <limits>
#include "rounding_mode.h"

namespace job::simd  {

using f32 = __m256;
using i32 = __m256i;

// Floats
struct AVX_F {
    static constexpr inline int width()
    {
        return 8;
    }

    // set a register bit to zero
    static inline f32 zero()
    {
        return _mm256_setzero_ps();
    }

    // set value of a register
    static inline f32 set1(float v)
    {
        return _mm256_set1_ps(v);
    }

    static inline i32 set1_i32(int v)
    {
        return _mm256_set1_epi32(v);
    }


    // Manual Quiet NaN for Fast-Math safety (0x7FC00000)
    static inline f32 qnan()
    {
        return _mm256_castsi256_ps(_mm256_set1_epi32(0x7FC00000));
    }

    // get value of a register
    static inline f32 pull(const float *p)
    {
        return _mm256_loadu_ps(p);
    }
    // move a float to a register
    static inline void mov(float *p, f32 reg)
    {
        _mm256_storeu_ps(p, reg);
    }

    // +
    static inline f32 add(f32 reg_a, f32 reg_b)
    {
        return _mm256_add_ps(reg_a, reg_b);
    }

    static inline i32 add_i32(i32 reg_a, i32 reg_b)
    {
        return _mm256_add_epi32(reg_a, reg_b);
    }

    //
    static inline f32 sub(f32 reg_a, f32 reg_b)
    {
        return _mm256_sub_ps(reg_a, reg_b);
    }

    static inline i32 sub_i32(i32 reg_a, i32 reg_b)
    {
        return _mm256_sub_epi32(reg_a, reg_b);
    }

    // multiple
    static inline f32 mul(f32 reg_a, f32 reg_b)
    {
        return _mm256_mul_ps(reg_a, reg_b);
    }

    // divsion
    static inline f32 div(f32 reg_a, f32 reg_b)
    {
        return _mm256_div_ps(reg_a, reg_b);
    }

    // (reg_a * reg_b) + reg_c
    static inline f32 mul_plus(f32 reg_a, f32 reg_b, f32 reg_c)
    {
        return _mm256_fmadd_ps(reg_a, reg_b, reg_c);
    }

    // Adds the even-indexed values and subtracts the odd-indexed values
    static inline f32 addsub(f32 reg_a, f32 reg_b)
    {
        return _mm256_addsub_ps(reg_a, reg_b);
    }

    static inline f32 max(f32 reg_a, f32 reg_b)
    {
        return _mm256_max_ps(reg_a, reg_b);
    }

    static inline f32 min(f32 reg_a, f32 reg_b)
    {
        return _mm256_min_ps(reg_a, reg_b);
    }

    // Square root
    static inline f32 sqrt(f32 reg)
    {
        return _mm256_sqrt_ps(reg);
    }
    // reciprocal square root
    static inline f32 rsqrt(f32 reg)
    {
        return _mm256_rsqrt_ps(reg);
    }

    // round
    template<RoundingMode Mode>
    __attribute__((always_inline))
    static inline f32 round(f32 reg_a)
    {
        return _mm256_round_ps(reg_a, (int)(Mode));
    }

    static inline f32 ceil(f32 reg_a)
    {
        return _mm256_ceil_ps(reg_a);
    }

    static inline f32 floor(f32 reg_a)
    {
        return _mm256_floor_ps(reg_a);
    }

    // VANDPS
    static inline f32 eq(f32 reg_a, f32 reg_b)
    {
        return _mm256_and_ps(reg_a, reg_b);
    }

    static inline i32 srli_epi32(i32 a, int imm)
    {
        return _mm256_srli_epi32(a, imm);
    }

    static inline f32 is_not(f32 reg_a, f32 reg_b)
    {
        return _mm256_andnot_ps(reg_a, reg_b);
    }

    static inline f32 gt_ps(f32 a, f32 b)
    {
        return _mm256_cmp_ps(a, b, _CMP_GT_OQ);
    }

    static inline f32 le_ps(f32 a, f32 b)
    {
        return _mm256_cmp_ps(a, b, _CMP_LE_OQ);
    }

    static inline f32 blendv(f32 a, f32 b, f32 mask)
    {
        return _mm256_blendv_ps(a, b, mask);
    }

    // Or
    static inline f32 or_ps(f32 reg_a, f32 reg_b)
    {
        return _mm256_or_ps(reg_a, reg_b);
    }

    static inline f32 and_ps(f32 reg_a, f32 reg_b)
    {
        return _mm256_and_ps(reg_a, reg_b);
    }

    // Integer Bitwise OR
    static inline i32 or_si(i32 a, i32 b)
    {
        return _mm256_or_si256(a, b);
    }

    // Integer Bitwise AND (Needed for mantissa masking)
    static inline i32 and_si(i32 a, i32 b)
    {
        return _mm256_and_si256(a, b);
    }

    static inline f32 xor_ps(f32 reg_a, f32 reg_b)
    {
        return _mm256_xor_ps(reg_a, reg_b);
    }

    //Horizontally adds the adjacent pairs of values contained in two/ (VHADDPS)
    static inline f32 hadd(f32 reg_a, f32 reg_b)
    {
        return _mm256_hadd_ps(reg_a, reg_b);
    }
    //Horizontally adds the adjacent pairs of values contained in two/ (VHADDPS)
    static inline f32 hsub(f32 reg_a, f32 reg_b)
    {
        return _mm256_hsub_ps(reg_a, reg_b);
    }

    template<typename T>
    static inline f32 blend(f32 reg_a, f32 reg_b, T x)
    {
        return _mm256_blend_ps(reg_a, reg_b, (int)x);
    }

    template<int Mask>
    static inline f32 shuffle(f32 reg_a, f32 reg_b)
    {
        return _mm256_shuffle_ps(reg_a, reg_b, Mask);
    }

    // Interleave lower halves (Unpack Lo)
    // A: [a0 a1 a2 a3 ...] B: [b0 b1 b2 b3 ...] -> [a0 b0 a1 b1 ...]
    static inline f32 unpack_lo(f32 a, f32 b)
    {
        return _mm256_unpacklo_ps(a, b);
    }

    // Interleave upper halves (Unpack Hi)
    static inline f32 unpack_hi(f32 a, f32 b)
    {
        return _mm256_unpackhi_ps(a, b);
    }

    // Permute 128-bit lanes (AVX specific, but we expose it for 8-wide logic)
    // 0x20: Lo(A), Lo(B) | 0x31: Hi(A), Hi(B)
    template<int Mask>
    static inline f32 permute_lanes(f32 a, f32 b)
    {
        return _mm256_permute2f128_ps(a, b, Mask);
    }

    static inline i32 cast_to_int(f32 reg)
    {
        return _mm256_castps_si256(reg);
    }
    static inline f32 cast_to_float(i32 reg)
    {
        return _mm256_castsi256_ps(reg);
    }

    static inline i32 cvt_f32_i32(f32 reg)
    {
        return _mm256_cvttps_epi32(reg);
    }
    static inline f32 cvt_i32_f32(i32 reg)
    {
        return _mm256_cvtepi32_ps(reg);
    }

    static inline i32 slli_epi32(i32 a, int imm)
    {
        return _mm256_slli_epi32(a, imm);
    }

    static inline f32 clamp_f32(f32 x, float floor, float ceil)
    {
        return max(set1(floor), min(set1(ceil), x));
    }

    // EXP
    static inline f32 exp_schraudolph(f32 x)
    {
        // clamp x to prevent overflow/underflow (-87 to 87)
        auto lo = set1(-87.0f);
        auto hi = set1(87.0f);
        x = max(lo, min(hi, x));

        // magic scale: 2^23 / ln(2)
        auto a = set1(12102203.16f);

        // bias adjustment: 127 * 2^23 - correction
        auto b = set1_i32(1064986823);

        // y = x * (2^23 / ln(2))
        auto product = mul(x, a);

        auto i_product = cvt_f32_i32(product);

        auto i_result = add_i32(i_product, b);

        return cast_to_float(i_result);
    }

    // useful if schraudolph's stepwise nature causes quantization noise.
    static inline f32 exp_poly5(f32 x)
    {
        auto c0 = set1(1.0f);
        auto c1 = set1(1.0f);
        auto c2 = set1(0.5f);
        auto c3 = set1(0.166666667f);
        auto c4 = set1(0.041666667f);
        auto c5 = set1(0.008333333f);

        x = max(set1(-80.0f), min(set1(80.0f), x));

        // Horner's Scheme
        auto term = mul_plus(x, c5, c4);
        term = mul_plus(x, term, c3);
        term = mul_plus(x, term, c2);
        term = mul_plus(x, term, c1);
        term = mul_plus(x, term, c0);
        return term;
    }
    // --- Complex Math: Exponential (e^x) ---
    // Uses Estrin's Scheme (Degree 6) for maximum Instruction Level Parallelism
    static inline f32 exp_estrin(f32 x)
    {
        //    exp(88) overflows float, exp(-88) underflows to zero
        auto lo = set1(-87.5f);
        auto hi = set1(87.5f);
        x = max(lo, min(hi, x));

        auto log2_e = set1(1.4426950408f); // 1/ln(2)
        auto k_f = round<RoundingMode::Nearest>(mul(x, log2_e));
        auto k = cvt_f32_i32(k_f);

        // r = x - k * ln(2)
        // We use extended precision (Hi/Lo) to avoid precision loss when x is large
        auto ln2_hi = set1(0.6931471806f);
        auto ln2_lo = set1(1.4286068203e-6f);

        auto r = mul_plus(k_f, set1(-1.0f) * ln2_hi, x); // r = x - k*ln2_hi
        r = mul_plus(k_f, set1(-1.0f) * ln2_lo, r);      // r = r - k*ln2_lo

        auto r2 = mul(r, r);
        auto r4 = mul(r2, r2);

        // Layer 1: Independent FMAs
        // P01 = (1 * r) + 1
        auto p01 = mul_plus(set1(1.0f), r, set1(1.0f));

        // P23 = (1/6 * r) + 1/2
        auto p23 = mul_plus(set1(0.166666667f), r, set1(0.5f));

        // P45 = (1/120 * r) + 1/24
        auto p45 = mul_plus(set1(0.008333333f), r, set1(0.041666667f));

        // Layer 2: Combine
        // P03 = P23 * r^2 + P01
        auto p03 = mul_plus(p23, r2, p01);

        // P46 = (1/720) * r^2 + P45
        auto p46 = mul_plus(set1(0.001388889f), r2, p45);

        // Layer 3: Final Poly
        // Poly = P46 * r^4 + P03
        auto poly = mul_plus(p46, r4, p03);

        // 4. Reconstruction: Result = poly * 2^k
        //    We add 'k' directly to the exponent bits of the float.
        auto exponent_shift = slli_epi32(k, 23);
        auto res_int = cast_to_int(poly);
        auto final_int = add_i32(res_int, exponent_shift);

        return cast_to_float(final_int);
    }

    // --- Complex Math: Natural Logarithm (ln) ---
    static inline f32 log(f32 x)
    {

        auto one = set1(1.0f);
        auto zero = set1(0.0f);
        auto half = set1(0.5f);

        // x <= 0
        auto invalid_mask = le_ps(x, zero);

        // x = m * 2^k
        auto x_int = cast_to_int(x);

        // k = (exponent_bits >> 23) - 127
        // !!Note!!: Using raw intrinsic for shift because wrapper might be templated in some implementations ?
        auto k_raw = sub_i32(srli_epi32(x_int, 23), set1_i32(0x7F) );
        auto k = cvt_i32_f32(k_raw);

        // m = (x & 0x7FFFFF) | 0x3F800000 (Force exponent to 1.0f)
        auto m_mask = set1_i32(0x7FFFFF);
        auto one_bits = set1_i32(0x3F800000);
        auto m_int = or_si(and_si(x_int, m_mask), one_bits );
        auto m = cast_to_float(m_int); // m is now in [1.0, 2.0)

        // If m > sqrt(2) -> shift: m = m * 0.5, k = k + 1
        auto sqrt2 = set1(1.41421356f);

        // 1.0 where m > sqrt(2)
        auto mask_scale = gt_ps(m, sqrt2);

        // m = blend(m, m * 0.5, mask_scale)
        auto m_scaled = mul(m, half);
        m = blendv(m, m_scaled, mask_scale);

        // k = k + (mask_scale ? 1.0 : 0.0)
        auto k_correction = and_ps(mask_scale, one);
        k = add(k, k_correction);

        // polynomial: z = m - 1
        auto z = sub(m, one);

        // Estrin's scheme (degree 8) minimax polynomial for reduced range
        auto C0 = set1(-0.5f);
        auto C1 = set1( 0.3333333134f);
        auto C2 = set1(-0.2500000000f);
        auto C3 = set1( 0.2000000030f);
        auto C4 = set1(-0.1666666667f);
        auto C5 = set1( 0.1428571492f);
        auto C6 = set1(-0.1250000000f);
        auto C7 = set1( 0.1111111119f);

        auto z2 = mul(z, z);
        auto z4 = mul(z2, z2);

        auto p01 = mul_plus(C1, z, C0);
        auto p23 = mul_plus(C3, z, C2);
        auto p45 = mul_plus(C5, z, C4);
        auto p67 = mul_plus(C7, z, C6);

        auto p03 = mul_plus(p23, z2, p01);
        auto p47 = mul_plus(p67, z2, p45);

        auto poly = mul_plus(p47, z4, p03);

        // final reconstruction: log(x) = k*ln(2) + z*(1 + z*poly)
        auto ln2_hi = set1(0.6931471806f);

        auto r = mul_plus(poly, z, one);
        auto poly_term = mul(r, z);

        auto res = mul_plus(k, ln2_hi, poly_term);

        // handle invalid inputs (fast-math strikes again)
        return blendv(res, qnan(), invalid_mask);
    }

    static inline f32 exp(f32 x)
    {
        return exp_estrin(x);
    }

    static inline f32 exp_fast(f32 x)
    {
        return exp_schraudolph(x);
    }

};


}
