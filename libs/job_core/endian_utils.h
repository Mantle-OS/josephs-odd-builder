#pragma once
#include <concepts>
#include <cstdint>
#include <bit>

namespace job::core {

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
#include <bit>
#define JOB_HAS_NATIVE_BYTESWAP 1
#else
#define JOB_HAS_NATIVE_BYTESWAP 0
#endif

template <std::integral T>
constexpr T byteswap_internal(T v) noexcept {
#if JOB_HAS_NATIVE_BYTESWAP
    return std::byteswap(v);
#else
    // C++20 Fallback using GCC/Clang builtins (Linux-specific as requested)
    if constexpr (sizeof(T) == 1)
        return v;
    else if constexpr (sizeof(T) == 2)
        return __builtin_bswap16(v);
    else if constexpr (sizeof(T) == 4)
        return __builtin_bswap32(v);
    else if constexpr (sizeof(T) == 8)
        return __builtin_bswap64(v);
#endif
}

constexpr uint16_t toLE16(uint16_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return byteswap_internal(v);
}

constexpr uint16_t fromLE16(uint16_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return byteswap_internal(v);
}

constexpr uint32_t toLE32(uint32_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return byteswap_internal(v);
}

constexpr uint32_t fromLE32(uint32_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return byteswap_internal(v);
}

}
