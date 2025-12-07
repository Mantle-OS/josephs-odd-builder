#pragma once

namespace job::core {

#ifdef JOB_CORE_USE_DOUBLE
    typedef double real_t;
#else
    typedef float real_t;
#endif

inline constexpr real_t piToTheFace = real_t(3.141592653589793238462643383279502884L);

}
