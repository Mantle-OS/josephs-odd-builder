#pragma once

#include <cstdint>

#include <linux/fb.h>

namespace job::render {

struct JobFbBitfield
{
    std::uint32_t   offset{};
    std::uint32_t   length{};
    bool            msbRight{};

    [[nodiscard]] static constexpr JobFbBitfield fromFb(const fb_bitfield &field) noexcept
    {
        return JobFbBitfield{ field.offset, field.length, field.msb_right != 0 };
    }

    [[nodiscard]] constexpr fb_bitfield toFb() const noexcept
    {
        return fb_bitfield{
            .offset     = offset,
            .length     = length,
            .msb_right  = msbRight ? 1u : 0u
        };
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return length == 0;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return length <= 32 && offset <= 32 && offset + length <= 32;
    }

    [[nodiscard]] constexpr std::uint32_t end() const noexcept
    {
        return offset + length;
    }

    bool operator==(const JobFbBitfield &) const noexcept = default;
};

}