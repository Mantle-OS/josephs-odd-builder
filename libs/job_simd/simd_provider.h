#pragma once

#if defined(HAS_AVX)
#include <immintrin.h>
#if defined(__AVX512F__)
#include "simd_avx512.h" // 16 width floats This file does not exsist yet
#elif defined(__AVX2__)
#include "simd_avx.h" // 8 width floats
#endif
namespace job::simd  {
using SIMD = AVX_F;
}
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#include "simd_arm.h" // arm_neon.h
namespace job::simd {
using SIMD = NEON_F;
}
#else
#error "JobAi requires AVX2 AVX512F or NEON support. Please check your compiler flags."
#endif
#include "simd_math.h"
namespace job::simd {
inline f32 SIMD::exp(f32 x)      { return exp_estrin(x); }
inline f32 SIMD::log(f32 x)      { return log(x); }
inline f32 SIMD::exp_fast(f32 x) { return exp_schraudolph(x); }
}
