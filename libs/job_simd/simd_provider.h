#pragma once

#if defined(HAS_AVX_512) || defined(HAS_AVX_512_VNNI)
#include "simd_avx512.h" // 16 width floats This file does not exsist yet
namespace job::simd {
using SIMD = AVX512_F;
}

#elif defined(HAS_AVX) || defined(HAS_AVX_TWO) || defined(HAS_AVX_VNNI)

#include "simd_avx.h" // 8 width floats
namespace job::simd {
using SIMD = AVX_F;
}

#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#include "simd_arm.h" // arm_neon.h

namespace job::simd {
using SIMD = NEON_F;
}

#else
#error "JobAi requires AVX, AVX_512, AVX_VINNI or NEON support. Please check your compiler flags. See cmake/check_hardware.cmake"
#endif


// after we have gathered the namespcae
#include "simd_math.h"

namespace job::simd {
inline f32 SIMD::exp(f32 x)      { return exp_estrin(x); }
inline f32 SIMD::log(f32 x)      { return avx_log(x); }
inline f32 SIMD::exp_fast(f32 x) { return exp_schraudolph(x); }
inline i64 SIMD::siphash(i64 m0, i64 m1, i64 k0, i64 k1) noexcept
{
    return avx_siphash(m0, m1, k0, k1);
}
inline i64 SIMD::siphashTile4(const std::uint64_t *uids, std::uint64_t k0, std::uint64_t k1) noexcept
{
    return avx_siphash_tile4(uids, k0, k1);
}

}