#pragma once

#include <immintrin.h>
#include <limits>
#include "rounding_mode.h"

namespace job::simd {

using f32 = __m512;
using i32 = __m512i;

struct AVX512_F {
    // Width Size for the kernels
    static constexpr inline int width() { return 16; }
    // getters, setters, init
    static inline f32 zero()                    { return _mm512_setzero_ps(); }
    static inline f32 set1(float v)             { return _mm512_set1_ps(v); }
    static inline i32 set1_i32(int v)           { return _mm512_set1_epi32(v); }
    static inline f32 pull(const float *p)      { return _mm512_loadu_ps(p); }
    static inline void mov(float *p, f32 reg)   { _mm512_storeu_ps(p, reg); }
    // Basic Float Math
    static inline f32 add(f32 a, f32 b)             { return _mm512_add_ps(a, b); }
    static inline f32 sub(f32 a, f32 b)             { return _mm512_sub_ps(a, b); }
    static inline f32 mul(f32 a, f32 b)             { return _mm512_mul_ps(a, b); }
    static inline f32 div(f32 a, f32 b)             { return _mm512_div_ps(a, b); }
    static inline f32 mul_plus(f32 a, f32 b, f32 c) { return _mm512_fmadd_ps(a, b, c); }
    // Basic Int Math
    static inline i32 add_i32(i32 a, i32 b) { return _mm512_add_epi32(a, b); }
    static inline i32 sub_i32(i32 a, i32 b) { return _mm512_sub_epi32(a, b); }
    static inline i32 and_si(i32 a, i32 b)  { return _mm512_and_si512(a, b); }
    static inline i32 or_si(i32 a, i32 b)   { return _mm512_or_si512(a, b); }
    static inline i32 slli_epi32(i32 a, int imm) { return _mm512_slli_epi32(a, imm); }
    static inline i32 srli_epi32(i32 a, int imm) { return _mm512_srli_epi32(a, imm); }
    // Casting
    static inline f32 qnan()                     { return _mm512_castsi512_ps(_mm512_set1_epi32(0x7FC00000)); }
    static inline __mmask16 ps_to_mask(f32 mask) { return _mm512_movepi32_mask(_mm512_castps_si512(mask)); }
    static inline f32 mask_to_ps(__mmask16 m)    { return _mm512_castsi512_ps(_mm512_movm_epi32(m)); }
    static inline i32 cast_to_int(f32 reg)       { return _mm512_castps_si512(reg); }
    static inline f32 cast_to_float(i32 reg) { return _mm512_castsi512_ps(reg); }
    static inline i32 cvt_f32_i32(f32 reg)   { return _mm512_cvttps_epi32(reg); }
    static inline f32 cvt_i32_f32(i32 reg)   { return _mm512_cvtepi32_ps(reg); }
    // Logic
    static inline f32 max(f32 a, f32 b)    { return _mm512_max_ps(a, b); }
    static inline f32 min(f32 a, f32 b)    { return _mm512_min_ps(a, b); }
    static inline f32 eq(f32 a, f32 b)     { return _mm512_and_ps(a, b); }
    static inline f32 and_ps(f32 a, f32 b) { return _mm512_and_ps(a, b); }
    static inline f32 or_ps(f32 a, f32 b)  { return _mm512_or_ps(a, b); }
    static inline f32 xor_ps(f32 a, f32 b) { return _mm512_xor_ps(a, b); }
    static inline f32 is_not(f32 a, f32 b) { return _mm512_andnot_ps(a, b); }
    // Compair
    static inline f32 gt_ps(f32 a, f32 b) { return mask_to_ps(_mm512_cmp_ps_mask(a, b, _CMP_GT_OQ)); }
    static inline f32 le_ps(f32 a, f32 b) { return mask_to_ps(_mm512_cmp_ps_mask(a, b, _CMP_LE_OQ)); }
    static inline f32 blendv(f32 a, f32 b, f32 mask)
    {
        __mmask16 k = ps_to_mask(mask);
        return _mm512_mask_blend_ps(k, a, b);
    }


    // Rounding
    template<RoundingMode Mode>
    static inline f32 round(f32 a) { return _mm512_roundscale_ps(a, (int)Mode); }
    //
    static inline f32 unpack_lo(f32 a, f32 b) { return _mm512_unpacklo_ps(a, b); }
    static inline f32 unpack_hi(f32 a, f32 b) { return _mm512_unpackhi_ps(a, b); }

    template<int Mask>
    static inline f32 shuffle(f32 a, f32 b) { return _mm512_shuffle_ps(a, b, Mask); }

    // complex Math (See simd_provider.h -> simd_math.h)
    static f32 exp(f32 x);
    static f32 log(f32 x);
    static f32 exp_fast(f32 x);
};

} // namespace job::simd
