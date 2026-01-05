#pragma once
namespace job::simd  {


#if defined(__AVX512F__) || defined(__AVX2__)

#include <immintrin.h>

enum class RoundingMode : int {
    Nearest  = _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC, // 0x08
    Down     = _MM_FROUND_TO_NEG_INF     | _MM_FROUND_NO_EXC, // 0x09
    Up       = _MM_FROUND_TO_POS_INF     | _MM_FROUND_NO_EXC, // 0x0A
    Truncate = _MM_FROUND_TO_ZERO        | _MM_FROUND_NO_EXC  // 0x0B
};
#elif defined(__ARM_NEON)
enum class RoundingMode : int {
    Nearest  = 0,
    Down     = 1, // Floor
    Up       = 2, // Ceil
    Truncate = 3
};
#endif

}
