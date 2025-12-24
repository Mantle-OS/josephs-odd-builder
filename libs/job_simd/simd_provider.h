#pragma once
#if defined(HAS_AVX)
#include <immintrin.h>

#if defined(__AVX512F__)
#include "simd_avx512.h" // 16 width floats
#elif defined(__AVX2__)
#include "simd_avx.h" // 8 width floats
#endif
namespace job::simd  {
using SIMD = AVX_F;
}
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#include "simd_arm.h" // arm_neon.h
namespace job::ai::comp {
using SIMD = NEON_F;
}
#else
  #error "JobAi requires AVX2 or NEON support. Please check your compiler flags."
#endif
