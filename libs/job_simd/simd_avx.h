#pragma once
#include <immintrin.h>
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
#if defined(__FMA__)
        return _mm256_fmadd_ps(reg_a, reg_b, reg_c);
#else
        // Fallback: (a * b) + c
        return _mm256_add_ps(_mm256_mul_ps(reg_a, reg_b), reg_c);
#endif
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


    // complex Math (See simd_provider.h -> simd_math.h)
    static f32 exp(f32 x);
    static f32 log(f32 x);
    static f32 exp_fast(f32 x);
};


}
