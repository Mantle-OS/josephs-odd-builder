#pragma once

#include <cstdint>
#include <immintrin.h>
#include "rounding_mode.h"
namespace job::simd  {

using u8 = __m256i;
using f16 = __m128;
using u16 = __m256i;
using u32 = __m256i;
using f32 = __m256;
using i32 = __m256i;
using i64 = __m256i;

// Floats
struct AVX_F {
    // Width Size for the kernels
    static constexpr inline int width() { return 8; }
    // getters, setters, init
    static inline f32 zero() {                  return _mm256_setzero_ps(); }
    static inline f32 set1(float v) {           return _mm256_set1_ps(v); }
    static inline i32 set1_i32(int v) {         return _mm256_set1_epi32(v); }
    static inline f32 pull(const float *p) {    return _mm256_loadu_ps(p); }
    static inline void mov(float *p, f32 reg) { _mm256_storeu_ps(p, reg); }

    template<int Mask>
    static inline f32 permute_lanes(f32 a, f32 b) { return _mm256_permute2f128_ps(a, b, Mask); }

    // Basic Float Math
    static inline f32 add(f32 reg_a, f32 reg_b) { return _mm256_add_ps(reg_a, reg_b); }
    static inline f32 sub(f32 reg_a, f32 reg_b) { return _mm256_sub_ps(reg_a, reg_b); }
    static inline f32 mul(f32 reg_a, f32 reg_b) { return _mm256_mul_ps(reg_a, reg_b); }
    static inline f32 div(f32 reg_a, f32 reg_b) { return _mm256_div_ps(reg_a, reg_b); }

    static inline f32 addsub(f32 reg_a, f32 reg_b) { return _mm256_addsub_ps(reg_a, reg_b); }

    static inline f32 sqrt(f32 reg) { return _mm256_sqrt_ps(reg); }
    static inline f32 rsqrt(f32 reg) { return _mm256_rsqrt_ps(reg); }

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

    // Basic Int Math
    static inline i32 add_i32(i32 reg_a, i32 reg_b) { return _mm256_add_epi32(reg_a, reg_b); }
    static inline i32 sub_i32(i32 reg_a, i32 reg_b) { return _mm256_sub_epi32(reg_a, reg_b); }
    static inline i32 and_si(i32 a, i32 b) { return _mm256_and_si256(a, b);}
    static inline i32 or_si(i32 a, i32 b) { return _mm256_or_si256(a, b); }
    static inline i32 slli_epi32(i32 a, int imm) { return _mm256_slli_epi32(a, imm); }
    static inline i32 srli_epi32(i32 a, int imm) { return _mm256_srli_epi32(a, imm); }

    // Casting
    static inline f32 qnan() { return _mm256_castsi256_ps(_mm256_set1_epi32(0x7FC00000)); }
    // static inline __mmask16 ps_to_mask(f32 mask) { return _mm512_movepi32_mask(_mm256_castps_si256(mask)); }
    // static inline f32 mask_to_ps(__mmask16 m)    { return _mm512_castsi512_ps(_mm512_movm_epi32(m)); }
    static inline i32 cast_to_int(f32 reg)  { return _mm256_castps_si256(reg); }
    static inline f32 cast_to_float(i32 reg){ return _mm256_castsi256_ps(reg); }
    static inline f32 cvt_i32_f32(i32 reg)  { return _mm256_cvtepi32_ps(reg); }
    static inline i32 cvt_f32_i32(f32 reg)  { return _mm256_cvttps_epi32(reg); }


    static inline  f16 cast_f32_f16(f32 reg){return _mm256_castps256_ps128(reg);}

    // template<typename T>
    // static inline  f16 ext_f16(f32 a, const T b){return _mm256_extractf128_ps(a, (int)b);}
    template<typename T>
    [[nodiscard]] static inline f16 ext_f16(f32 a, T b) { return static_cast<int>(b) == 0 ? _mm256_castps256_ps128(a) : _mm256_extractf128_ps(a, 1); }
    static inline  f16 add_f16(f16 a, f16 b){ return _mm_add_ps(a, b);}

    // Logic
    static inline f32 max(f32 reg_a, f32 reg_b) { return _mm256_max_ps(reg_a, reg_b); }
    static inline f32 min(f32 reg_a, f32 reg_b) { return _mm256_min_ps(reg_a, reg_b); }
    static inline f32 eq(f32 reg_a, f32 reg_b)     { return _mm256_cmp_ps(reg_a, reg_b, _CMP_EQ_OQ); }
    static inline f32 and_ps(f32 reg_a, f32 reg_b) { return _mm256_and_ps(reg_a, reg_b); }
    static inline f32 or_ps(f32 reg_a, f32 reg_b) { return _mm256_or_ps(reg_a, reg_b); }
    static inline f32 xor_ps(f32 reg_a, f32 reg_b) { return _mm256_xor_ps(reg_a, reg_b); }
    static inline f32 is_not(f32 reg_a, f32 reg_b) { return _mm256_andnot_ps(reg_a, reg_b); }

    static inline f32 clamp_f32(f32 x, float floor, float ceil) { return max(set1(floor), min(set1(ceil), x)); }

    // Compair
    static inline f32 gt_ps(f32 a, f32 b) { return _mm256_cmp_ps(a, b, _CMP_GT_OQ); }
    static inline f32 le_ps(f32 a, f32 b) { return _mm256_cmp_ps(a, b, _CMP_LE_OQ); }
    static inline f32 blendv(f32 a, f32 b, f32 mask) { return _mm256_blendv_ps(a, b, mask); }
    template<typename T>
    static inline f32 blend(f32 reg_a, f32 reg_b, T x) { return _mm256_blend_ps(reg_a, reg_b, (int)x); }

    // yikes
    static inline f32 hadd(f32 reg_a, f32 reg_b) { return _mm256_hadd_ps(reg_a, reg_b); }
    static inline f32 hsub(f32 reg_a, f32 reg_b) { return _mm256_hsub_ps(reg_a, reg_b); }

    // Rounding
    static inline f32 ceil(f32 reg_a)  { return _mm256_ceil_ps(reg_a); }
    static inline f32 floor(f32 reg_a) { return _mm256_floor_ps(reg_a);}

    template<RoundingMode Mode>
    __attribute__((always_inline))
    static inline f32 round(f32 reg_a) { return _mm256_round_ps(reg_a, (int)(Mode)); }

    static inline f32 unpack_lo(f32 a, f32 b) { return _mm256_unpacklo_ps(a, b); }
    static inline f32 unpack_hi(f32 a, f32 b) { return _mm256_unpackhi_ps(a, b); }

    template<int Mask>
    static inline f32 shuffle(f32 reg_a, f32 reg_b) { return _mm256_shuffle_ps(reg_a, reg_b, Mask); }


    ///////////////// i64 //////////////////////////////

    static inline i64 zero_i64() {                              return _mm256_setzero_si256(); }
    static inline i64 set1_i64(std::int64_t v) {                return _mm256_set1_epi64x(v); }
    static inline i64 set1_u64(std::uint64_t v) {                return _mm256_set1_epi64x(static_cast<std::int64_t>(v)); }
    static inline i64 pull_i64(const std::int64_t *p) {         return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    static inline void mov_i64(std::int64_t *p, i64 reg) {      _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), reg); }

    static inline i64 add_i64(i64 reg_a, i64 reg_b) {           return _mm256_add_epi64(reg_a, reg_b); }
    static inline i64 sub_i64(i64 reg_a, i64 reg_b) {           return _mm256_sub_epi64(reg_a, reg_b); }

    [[nodiscard]] static inline i64 xor_i64(i64 a, i64 b) {     return _mm256_xor_si256(a, b); }
    [[nodiscard]] static inline i64 or_i64(i64 a, i64 b) {      return _mm256_or_si256(a, b); }

    template<int Shift>
    [[nodiscard]] static inline i64 slli_i64(i64 a)
    {
        return _mm256_slli_epi64(a, Shift);
    }

    template<int Shift>
    [[nodiscard]] static inline i64 srli_i64(i64 a)
    {
        return _mm256_srli_epi64(a, Shift);
    }


    // ----------- JOB TEXT -----------
    // U8
    static constexpr inline int width_u8() { return 32; }
    [[nodiscard]] static inline u8 pull_u8(const std::uint8_t *p) {     return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    [[nodiscard]] static inline u8 set1_u8(std::uint8_t value) {        return _mm256_set1_epi8(static_cast<char>(value)); }
    [[nodiscard]] static inline u8 and_u8(u8 a, u8 b) {                 return _mm256_and_si256(a, b); }
    [[nodiscard]] static inline u8 or_u8(u8 a, u8 b) {                  return _mm256_or_si256(a, b); }
    [[nodiscard]] static inline u8 eq_u8(u8 a, u8 b) {                  return _mm256_cmpeq_epi8(a, b); }
    [[nodiscard]] static inline u8 shuffle_u8(u8 table, u8 indices) {   return _mm256_shuffle_epi8(table, indices); }
    [[nodiscard]] static inline std::uint32_t movemask_u8(u8 reg) {     return static_cast<std::uint32_t>(_mm256_movemask_epi8(reg)); }
    [[nodiscard]] static inline bool any_high_bit_u8(u8 reg) {          return movemask_u8(reg) != 0; }

    // U16
    static constexpr inline int width_u16() { return 16; }
    [[nodiscard]] static inline u16 pull_u16(const std::uint16_t *p) {  return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    [[nodiscard]] static inline u16 set1_u16(std::uint16_t value) {     return _mm256_set1_epi16(static_cast<std::int16_t>(value)); }
    [[nodiscard]] static inline u16 and_u16(u16 a, u16 b) {             return _mm256_and_si256(a, b); }
    [[nodiscard]] static inline u16 eq_u16(u16 a, u16 b) {              return _mm256_cmpeq_epi16(a, b); }

    template <int Shift>
    [[nodiscard]] static inline u16 srli_u16(u16 value)
    {
        static_assert(Shift >= 0 && Shift <= 15);
        return _mm256_srli_epi16(value, Shift);
    }

    [[nodiscard]] static inline std::uint32_t movemask_u16(u16 reg)
    {
        std::uint32_t mask = static_cast<std::uint32_t>(_mm256_movemask_epi8(reg));

        mask &= 0x55555555u;
        mask = (mask | (mask >> 1)) & 0x33333333u;
        mask = (mask | (mask >> 2)) & 0x0F0F0F0Fu;
        mask = (mask | (mask >> 4)) & 0x00FF00FFu;
        mask = (mask | (mask >> 8)) & 0x0000FFFFu;

        return mask;
    }

    // U32
    static constexpr inline int width_u32() { return 8; }
    [[nodiscard]] static inline u32 pull_u32(const std::uint32_t *p) {      return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    [[nodiscard]] static inline u32 set1_u32(std::uint32_t value) {         return _mm256_set1_epi32(static_cast<std::int32_t>(value)); }
    [[nodiscard]] static inline u32 and_u32(u32 a, u32 b) {                 return _mm256_and_si256(a, b); }
    [[nodiscard]] static inline u32 eq_u32(u32 a, u32 b) {                  return _mm256_cmpeq_epi32(a, b); }
    [[nodiscard]] static inline bool any_u32(u32 reg) {                     return !_mm256_testz_si256(reg, reg); }
    [[nodiscard]] static inline u32 or_u32(u32 a, u32 b) {                  return _mm256_or_si256(a, b); }
    [[nodiscard]] static inline u32 gt_u32(u32 a, u32 b)
    {
        const u32 sign = set1_u32(0x80000000u);
        return _mm256_cmpgt_epi32(_mm256_xor_si256(a, sign), _mm256_xor_si256(b, sign));
    }

    // I32
    // [[nodiscard]] static inline i32 gt_i32(i32 a, i32 b) { return _mm256_cmpgt_epi32(a, b); }
    // [[nodiscard]] static inline i32 xor_si(i32 a, i32 b) { return _mm256_xor_si256(a, b); }
    // [[nodiscard]] static inline i32 gt_u32(i32 a, i32 b) {
    //     const i32 sign = set1_i32(static_cast<int>(0x80000000u));
    //     return gt_i32(xor_si(a, sign), xor_si(b, sign));
    // }
    // [[nodiscard]] static inline bool any_i32(i32 reg) { return !_mm256_testz_si256(reg, reg); }

    // ------------- END JOB TEXT --------


    template<int Shift>
    [[nodiscard]] static inline i64 rotl_i64(i64 a)
    {
        return or_i64(slli_i64<Shift>(a), srli_i64<64 - Shift>(a));
    }
    static inline void loadSipHashTile4(const std::uint64_t *p, i64 &m0, i64 &m1) noexcept
    {
        // a = [ A0, A1, B0, B1 ]
        // b = [ C0, C1, D0, D1 ]
        const i64 a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
        const i64 b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p + 4));

        // Combine lower 128 bits: t0 = [ A0, A1, C0, C1 ]
        const i64 t0 = _mm256_permute2x128_si256(a, b, 0x20); // 0x20 = low(a), low(b)

        // Combine upper 128 bits: t1 = [ B0, B1, D0, D1 ]
        const i64 t1 = _mm256_permute2x128_si256(a, b, 0x31); // 0x31 = high(a), high(b)

        // Unpack 64-bit values across contiguous blocks:
        // m0 = [ A0, B0, C0, D0 ]
        // m1 = [ A1, B1, C1, D1 ]
        m0 = _mm256_unpacklo_epi64(t0, t1);
        m1 = _mm256_unpackhi_epi64(t0, t1);
    }

    // complex Math (See simd_provider.h -> simd_math.h)
    static f32 exp(f32 x);
    static f32 log(f32 x);
    static f32 exp_fast(f32 x);
    [[nodiscard]] static i64 siphash(i64 m0, i64 m1, i64 k0, i64 k1) noexcept;
    [[nodiscard]] static i64 siphashTile4(const std::uint64_t *uids, std::uint64_t k0, std::uint64_t k1) noexcept;
};
}
