#pragma once
#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>

namespace job::core {

#ifdef JOB_CORE_USE_DOUBLE
    typedef double real_t;
// Overload for double if you need it
[[nodiscard]] inline bool isSafeFinite(double d) noexcept
{
    std::uint64_t u = std::bit_cast<std::uint64_t>(d);

    // Exponent is 11 bits, mask 0x7FF0000000000000
    std::uint64_t exponent = (u & 0x7FF0000000000000ULL) >> 52;

    // 2047 (0x7FF) indicates Infinity or NaN
    return exponent != 2047;
}
#else
    typedef float real_t;
[[nodiscard]] inline bool isSafeFinite(float f) noexcept
{
    // Reinterpret bits as uint32_t
    std::uint32_t u = std::bit_cast<std::uint32_t>(f);

    // Extract Exponent (8 bits)
    std::uint32_t exponent = (u & 0x7F800000) >> 23;

    // 255 (0xFF) indicates Infinity or NaN
    return exponent != 255;
}
#endif

inline constexpr float piToTheFace = 3.141592653589793238462643383279502884L;

template <class S>
concept FloatScalar = std::floating_point<S>;

}
