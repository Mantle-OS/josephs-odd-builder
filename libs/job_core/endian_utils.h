#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace job::core {

template <std::integral T>
[[nodiscard]] constexpr T byteswapInternal(T value) noexcept
{
    static_assert(!std::is_same_v<T, bool>, "byteswapInternal: byte-swapping bool doesn't make sense");

    using UnsignedT = std::make_unsigned_t<T>;
    UnsignedT const unsignedValue = static_cast<UnsignedT>(value);

    if constexpr (sizeof(T) == 1) {
        return value;
    }
#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
    else {
        return static_cast<T>(std::byteswap(unsignedValue));
    }
#elif defined(_MSC_VER)
    else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(_byteswap_ushort(static_cast<unsigned short>(unsignedValue)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(_byteswap_ulong(static_cast<unsigned long>(unsignedValue)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(_byteswap_uint64(static_cast<unsigned __int64>(unsignedValue)));
    } else {
        static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "byteswapInternal: unsupported integral size for this platform's intrinsics");
    }
#elif defined(__GNUC__) || defined(__clang__)
    else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(__builtin_bswap16(static_cast<std::uint16_t>(unsignedValue)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(__builtin_bswap32(static_cast<std::uint32_t>(unsignedValue)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(__builtin_bswap64(static_cast<std::uint64_t>(unsignedValue)));
    } else {
        static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "byteswapInternal: unsupported integral size for this platform's intrinsics");
    }
#else
    else {
        UnsignedT result = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            result <<= 8;
            result |= static_cast<UnsignedT>((unsignedValue >> (i * 8)) & static_cast<UnsignedT>(0xFF));
        }
        return static_cast<T>(result);
    }
#endif
}

constexpr std::uint16_t toLE16(std::uint16_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return value;
    else
        return byteswapInternal(value);
}

constexpr std::uint16_t fromLE16(std::uint16_t value) noexcept
{
    return toLE16(value);
}

constexpr std::uint32_t toLE32(std::uint32_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return value;
    else
        return byteswapInternal(value);
}

constexpr std::uint32_t fromLE32(std::uint32_t value) noexcept
{
    return toLE32(value);
}

constexpr std::uint64_t toLE64(std::uint64_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return value;
    else
        return byteswapInternal(value);
}

constexpr std::uint64_t fromLE64(std::uint64_t value) noexcept
{
    return toLE64(value);
}

} // namespace job::core