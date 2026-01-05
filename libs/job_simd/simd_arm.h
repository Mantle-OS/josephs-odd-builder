#pragma once

#include <arm_neon.h>

namespace job::simd {
inline constexpr int kBlockSize = JOB_BLOCK_SIZE;

using f32 = float32x4_t;
using i32 = int32x4_t;

struct NEON_F {
    // 128-bit = 4 floats
    static constexpr int width()
    {
        return 4;
    }

    // Set Zero
    [[nodiscard]] static inline f32 zero()
    {
        return vdupq_n_f32(0.0f);
    }

    // Broadcast Scalar (renamed from push -> set1)
    [[nodiscard]] static inline f32 set1(float v)
    {
        return vdupq_n_f32(v);
    }

    static inline i32 set1_i32(int v)
    {
        return vdupq_n_s32(v);
    }


    // Load Unaligned (NEON handles unaligned loads well on ARMv8)
    [[nodiscard]] static inline f32 pull(const float *p)
    {
        return vld1q_f32(p);
    }

    // Load Aligned (Same intrinsic usually, but good for intent)
    [[nodiscard]] static inline f32 pull_a(const float *p)
    {
        return vld1q_f32(p);
    }

    // Store
    static inline void mov(float *p, f32 reg)
    {
        vst1q_f32(p, reg);
    }
    static inline void mov_a(float *p, f32 reg)
    {
        vst1q_f32(p, reg);
    }

    [[nodiscard]] static inline f32 add(f32 a, f32 b)
    {
        return vaddq_f32(a, b);
    }

    // Integer Add
    static inline i32 add_i32(i32 a, i32 b) {
        return vaddq_s32(a, b);
    }

    [[nodiscard]] static inline f32 sub(f32 a, f32 b)
    {
        return vsubq_f32(a, b);
    }
    [[nodiscard]] static inline f32 mul(f32 a, f32 b)
    {
        return vmulq_f32(a, b);
    }
    [[nodiscard]] static inline f32 div(f32 a, f32 b)
    {
        return vdivq_f32(a, b);
    }

    // FMA: (a * b) + c
    // Note: vfmaq_f32(c, a, b) performs c + (a * b)
    [[nodiscard]] static inline f32 mul_plus(f32 a, f32 b, f32 c)
    {
        return vfmaq_f32(c, a, b);
    }

    // --- Math Functions ---

    [[nodiscard]] static inline f32 max(f32 a, f32 b)
    {
        return vmaxq_f32(a, b);
    }
    [[nodiscard]] static inline f32 min(f32 a, f32 b)
    {
        return vminq_f32(a, b);
    }

    [[nodiscard]] static inline f32 sqrt(f32 a)
    {
        return vsqrtq_f32(a);
    }

    [[nodiscard]] static inline f32 rsqrt(f32 a)
    {
        return vrsqrteq_f32(a);
    }

    [[nodiscard]] static inline f32 and_ps(f32 a, f32 b)
    {
        return vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }

    [[nodiscard]] static inline f32 or_ps(f32 a, f32 b)
    {
        return vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }

    [[nodiscard]] static inline f32 xor_ps(f32 a, f32 b)
    {
        return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b)));
    }

    static inline f32 gt_ps(f32 a, f32 b)
    {
        return vreinterpretq_f32_u32(vcgtq_f32(a, b));
    }

    // vcleq_f32 returns 0xFFFFFFFF for true, 0 for false
    static inline f32 le_ps(f32 a, f32 b)
    {
        uint32x4_t res = vcleq_f32(a, b);
        return vreinterpretq_f32_u32(res);
    }

    static inline f32 blendv(f32 a, f32 b, f32 mask)
    {
        return vbslq_f32(vreinterpretq_u32_f32(mask), b, a);
    }

    // Quiet NaN constant
    static inline f32 qnan()
    {
        return vreinterpretq_f32_u32(vdupq_n_u32(0x7FC00000));
    }

    // ARMv8 provides specific instructions for rounding modes.
    template<int Mode>
    [[nodiscard]] __attribute__((always_inline))
    static inline f32 round(f32 a)
    {
        if constexpr (Mode == RoundingMode::Nearest)
            return vrndnq_f32(a); // Round to nearest, ties to even
        if constexpr (Mode == RoundingMode::Down)
            return vrndmq_f32(a); // Round towards -Inf (Floor)
        if constexpr (Mode == RoundingMode::Up)
            return vrndpq_f32(a); // Round towards +Inf (Ceil)
        if constexpr (Mode == RoundingMode::Truncate)
            return vrndq_f32(a);  // Round towards Zero
        return a;
    }

    [[nodiscard]] static inline f32 floor(f32 a)
    {
        return round<RoundingMode::Down>(a); }

    [[nodiscard]] static inline f32 ceil(f32 a)
    {
        return round<RoundingMode::Up>(a);
    }

    [[nodiscard]] static inline f32 trunc(f32 a)
    {
        return round<RoundingMode::Truncate>(a);
    }

    [[nodiscard]] static inline f32 round_nearest(f32 a)
    {
        return round<RoundingMode::Nearest>(a);
    }

    // Unpacking (Interleaving)
    // Corresponds to _mm_unpacklo_ps: [a0, b0, a1, b1]
    [[nodiscard]] static inline f32 unpack_lo(f32 a, f32 b)
    {
        return vzip1q_f32(a, b);
    }

    // Corresponds to _mm_unpackhi_ps: [a2, b2, a3, b3]
    [[nodiscard]] static inline f32 unpack_hi(f32 a, f32 b)
    {
        return vzip2q_f32(a, b);
    }

    template<int Mask>
    [[nodiscard]] static inline f32 shuffle(f32 a, f32 b)
    {
        if constexpr (Mask == 0x44)
            return vcombine_f32(vget_low_f32(a), vget_low_f32(b));

        if constexpr (Mask == 0xEE)
            return vcombine_f32(vget_high_f32(a), vget_high_f32(b));


        return a;
    }


    template<int Mask>
    [[nodiscard]] static inline f32 permute_lanes(f32 a, f32 /*b*/)
    {
        return a; // Should not be called in 4-wide mode
    }


    static inline i32 cast_to_int(f32 reg)
    {
        return vreinterpretq_s32_f32(reg);
    }

    static inline f32 cast_to_float(i32 reg)
    {
        return vreinterpretq_f32_s32(reg);
    }

    static inline i32 cvt_f32_i32(f32 reg)
    {
        return vcvtq_s32_f32(reg);
    }
    static inline f32 cvt_i32_f32(i32 reg)
    {
        return vcvtq_f32_s32(reg);
    }

    // Shift Left Immediate
    static inline i32 slli_epi32(i32 a, int imm)
    {
        // NEON requires constant immediate for shift intrinsic,
        // but 'int imm' is runtime variable in generic API signature.
        // However, generic code usually calls SIMD::slli_epi32(k, 23) where 23 is const.
        // We use a switch/case hack or GCC builtin if immediate is required.
        // For now, trusting the compiler to constant-fold or using vshlq_n_s32 if possible.
        return vshlq_s32(a, vdupq_n_s32(imm));
    }

    // Shift Right Logical Immediate (Zeros in)
    static inline i32 srli_epi32(i32 a, int imm)
    {
        // NEON 'vshlq_s32' with negative value does arithmetic shift (extends sign).
        // We need Logical shift (zeros). Use Unsigned reinterpretation.
        uint32x4_t u = vreinterpretq_u32_s32(a);
        // Note: variable shift 'vshlq_u32' takes negative for right shift
        uint32x4_t res = vshlq_u32(u, vdupq_n_s32(-imm));
        return vreinterpretq_s32_u32(res);
    }



    // --- Missing Float Logic (Parity with AVX) ---

    // is_not: (~a) & b
    static inline f32 is_not(f32 a, f32 b)
    {
        // NEON vbicq (Bit Clear) does: operand1 & (~operand2)
        // So we swap arguments: vbicq(b, a) -> b & (~a)
        return vreinterpretq_f32_u32(vbicq_u32(
            vreinterpretq_u32_f32(b),
            vreinterpretq_u32_f32(a)
            ));
    }


    // Integer subtract
    static inline i32 sub_i32(i32 a, i32 b)
    {
        return vsubq_s32(a, b);
    }

    // AVX's eq() is actually bitwise AND on floats (VANDPS in your comment)
    static inline f32 eq(f32 a, f32 b)
    {
        return vandq_f32(a, b);
    }


    // Integer bitwise
    static inline i32 or_si(i32 a, i32 b)
    {
        return vorrq_s32(a, b);
    }

    static inline i32 and_si(i32 a, i32 b)
    {
        return vandq_s32(a, b);
    }

    // addsub: AVX semantics are typically [a0-b0, a1+b1, a2-b2, a3+b3]
    static inline f32 addsub(f32 a, f32 b)
    {
        // Flip sign bit for lanes 0 and 2 in b, then add.
        uint32x4_t sign = vdupq_n_u32(0);
        sign = vsetq_lane_u32(0x80000000u, sign, 0);
        sign = vsetq_lane_u32(0x80000000u, sign, 2);

        uint32x4_t ub = vreinterpretq_u32_f32(b);
        f32 b2 = vreinterpretq_f32_u32(veorq_u32(ub, sign));
        return vaddq_f32(a, b2);
    }

    // Horizontal add: [a0+a1, a2+a3, b0+b1, b2+b3]
    static inline f32 hadd(f32 a, f32 b)
    {
        float32x2_t sa = vpadd_f32(vget_low_f32(a), vget_high_f32(a));
        float32x2_t sb = vpadd_f32(vget_low_f32(b), vget_high_f32(b));
        return vcombine_f32(sa, sb);
    }

    // Horizontal sub: [a0-a1, a2-a3, b0-b1, b2-b3]
    static inline f32 hsub(f32 a, f32 b)
    {
        // Turn subtraction into pairwise add by flipping odd lanes.
        const float32x4_t sign = { 1.0f, -1.0f, 1.0f, -1.0f };

        float32x4_t ta = vmulq_f32(a, sign);
        float32x4_t tb = vmulq_f32(b, sign);

        float32x2_t da = vpadd_f32(vget_low_f32(ta), vget_high_f32(ta));
        float32x2_t db = vpadd_f32(vget_low_f32(tb), vget_high_f32(tb));
        return vcombine_f32(da, db);
    }

    // Immediate blend (Mask bit i selects lane i from b, else from a)
    template<int Mask>
    static inline f32 blend(f32 a, f32 b)
    {
        uint32x4_t m = vdupq_n_u32(0);
        m = vsetq_lane_u32((Mask & 0x1) ? 0xFFFFFFFFu : 0u, m, 0);
        m = vsetq_lane_u32((Mask & 0x2) ? 0xFFFFFFFFu : 0u, m, 1);
        m = vsetq_lane_u32((Mask & 0x4) ? 0xFFFFFFFFu : 0u, m, 2);
        m = vsetq_lane_u32((Mask & 0x8) ? 0xFFFFFFFFu : 0u, m, 3);
        return vbslq_f32(m, b, a);
    }

    static inline f32 clamp_f32(f32 x, float lo, float hi)
    {
        return max(set1(lo), min(set1(hi), x));
    }



    // complex Math (See simd_provider.h -> simd_math.h)
    static f32 exp(f32 x);
    static f32 log(f32 x);
    static f32 exp_fast(f32 x);

};

} // namespace job::simd
