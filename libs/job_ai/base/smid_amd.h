#pragma once

#include <immintrin.h>

namespace job::ai {
enum class RoundingMode : int {
    Nearest  = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC, // 0x08
    Down     = _MM_FROUND_TO_NEG_INF     | _MM_FROUND_NO_EXC, // 0x09
    Up       = _MM_FROUND_TO_POS_INF     | _MM_FROUND_NO_EXC, // 0x0A
    Truncate = _MM_FROUND_TO_ZERO        | _MM_FROUND_NO_EXC  // 0x0B
};

#ifdef RYZEN_5950X
constexpr int kBlockSize = 256;
#else
inline constexpr int kBlockSize = 128;
#endif

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
    // -
    static inline f32 sub(f32 reg_a, f32 reg_b)
    {
        return _mm256_sub_ps(reg_a, reg_b);
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
    template<int Mode>
    __attribute__((always_inline))
    static inline f32 round(f32 reg_a)
    {
        return _mm256_round_ps(reg_a, Mode);
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

    static inline f32 is_not(f32 reg_a, f32 reg_b)
    {
        return _mm256_andnot_ps(reg_a, reg_b);
    }
    // Or
    static inline f32 or_ps(f32 reg_a, f32 reg_b)
    {
        return _mm256_xor_ps(reg_a, reg_b);
    }

    static inline f32 xor_ps(f32 reg_a, f32 reg_b)
    {
        return _mm256_or_ps(reg_a, reg_b);
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
};
} // namespace job::ai
