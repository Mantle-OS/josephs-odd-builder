#pragma once

#include <cstdint>

#include "job_color_channel.h"

namespace job::render {
static constexpr std::size_t kMaximumChannelCount = 4;
struct JobColorChannelInfo
{
    JobColorChannel     channel{JobColorChannel::None};
    std::uint8_t        bits{};
    std::uint8_t        shift{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return channel != JobColorChannel::None && bits != 0;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobColorChannelInfo &, const JobColorChannelInfo &) noexcept = default;

};

}
