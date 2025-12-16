#pragma once
#if defined(HAS_AVX)
#include <immintrin.h>
#include "simd_avx.h"
namespace job::simd  {
using SIMD = AVX_F;
}
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#include "simd_arm.h"
namespace job::ai::comp {
using SIMD = NEON_F;
}
#else
  #error "JobAi requires AVX2 or NEON support. Please check your compiler flags."
#endif
