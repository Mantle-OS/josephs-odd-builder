#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <limits>

// START FAST MATH WORKAROUNDS
template <std::floating_point T>
struct FloatBits;

template <>
struct FloatBits<float>
{
    using UInt = std::uint32_t;

    static constexpr UInt ExponentMask      = 0x7F800000U;
    static constexpr UInt MantissaMask      = 0x007FFFFFU;
    static constexpr UInt SignMask          = 0x80000000U;

    static constexpr UInt PositiveInfinity  = 0x7F800000U;
    static constexpr UInt QuietNaN          = 0x7FC00000U;
};

template <>
struct FloatBits<double>
{
    using UInt = std::uint64_t;

    static constexpr UInt ExponentMask      = 0x7FF0000000000000ULL;
    static constexpr UInt MantissaMask      = 0x000FFFFFFFFFFFFFULL;
    static constexpr UInt SignMask          = 0x8000000000000000ULL;

    static constexpr UInt PositiveInfinity  = 0x7FF0000000000000ULL;
    static constexpr UInt QuietNaN          = 0x7FF8000000000000ULL;
};

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr typename FloatBits<T>::UInt floatBits(T value) noexcept
{
    return std::bit_cast<typename FloatBits<T>::UInt>(value);
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr bool isSafeFinite(T value) noexcept
{
    using Bits = FloatBits<T>;
    return (floatBits(value) & Bits::ExponentMask) != Bits::ExponentMask;
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr bool isSafeInfinity(T value) noexcept
{
    using Bits = FloatBits<T>;
    const auto bits = floatBits(value);
    return (bits & Bits::ExponentMask) == Bits::ExponentMask &&
           (bits & Bits::MantissaMask) == 0;
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr bool isSafePositiveInfinity(T value) noexcept
{
    using Bits = FloatBits<T>;
    return floatBits(value) == Bits::PositiveInfinity;
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr bool isSafeNegativeInfinity(T value) noexcept
{
    using Bits = FloatBits<T>;
    return floatBits(value) == (Bits::PositiveInfinity | Bits::SignMask);
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr bool isSafeNaN(T value) noexcept
{
    using Bits = FloatBits<T>;
    const auto bits = floatBits(value);

    return (bits & Bits::ExponentMask) == Bits::ExponentMask &&
           (bits & Bits::MantissaMask) != 0;
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr T safeInfinity() noexcept
{
    return std::bit_cast<T>(FloatBits<T>::PositiveInfinity);
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr T safeNegativeInfinity() noexcept
{
    using Bits = FloatBits<T>;
    return std::bit_cast<T>(Bits::PositiveInfinity | Bits::SignMask);
}

template <std::floating_point T>
    requires (std::same_as<T, float> || std::same_as<T, double>)
[[nodiscard]] constexpr T safeNaN() noexcept
{
    return std::bit_cast<T>(FloatBits<T>::QuietNaN);
}

// END FAST MATH WORKAROUNDS