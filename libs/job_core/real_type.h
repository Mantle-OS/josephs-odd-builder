#pragma once

namespace job::core {

#ifdef JOB_CORE_USE_DOUBLE
    typedef double real_t;
#else
    typedef float real_t;
#endif

inline constexpr real_t piToTheFace = real_t(3.141592653589793238462643383279502884L);
inline constexpr real_t kSqrt2V     = real_t(1.41421356237309504880);
inline constexpr real_t kInvSqrt2   = real_t(0.70710678118654752440); // 1 / sqrt(2)
inline constexpr real_t kSqrt2DivPi = real_t(0.79788456080286535587989); // sqrt(2 / pi)
}
