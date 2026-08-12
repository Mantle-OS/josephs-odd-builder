#pragma once
#include <bit>
#include <concepts>
#include <cstdint>

namespace job::core {

[[nodiscard]] constexpr bool isSafeFinite(float f) noexcept
{
    std::uint32_t u = std::bit_cast<std::uint32_t>(f);
    std::uint32_t exponent = (u & 0x7F800000) >> 23;
    return exponent != 255;
}
[[nodiscard]] constexpr float safeInfinity() noexcept
{
    return std::bit_cast<float>(std::uint32_t{0x7F800000U});
}

[[nodiscard]] constexpr float safeNaN() noexcept
{
    return std::bit_cast<float>(std::uint32_t{0x7FC00000U});
}

inline constexpr float piToTheFace = 3.141592653589793238462643383279502884L;

template <class S>
concept FloatScalar = std::floating_point<S>;

}
