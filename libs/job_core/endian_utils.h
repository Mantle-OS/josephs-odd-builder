#pragma once
#include <cstdint>
#include <bit>

namespace job::core {

constexpr uint16_t toLE16(uint16_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return std::byteswap(v);
}

constexpr uint16_t fromLE16(uint16_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return std::byteswap(v);
}

constexpr uint32_t toLE32(uint32_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return std::byteswap(v);
}

constexpr uint32_t fromLE32(uint32_t v) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return v;
    else
        return std::byteswap(v);
}
}
