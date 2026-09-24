#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

#include "job_color_channel.h"

namespace job::render {

template <std::unsigned_integral ChannelType>
class BasicJobRgba
{
public:
    using Channel = ChannelType;

    constexpr BasicJobRgba() noexcept = default;

    constexpr BasicJobRgba(Channel red,
                           Channel green,
                           Channel blue,
                           Channel alpha = std::numeric_limits<Channel>::max()) noexcept :
        m_red(red),
        m_green(green),
        m_blue(blue),
        m_alpha(alpha)
    {
    }

    ~BasicJobRgba() = default;

    BasicJobRgba(const BasicJobRgba &) = default;
    BasicJobRgba &operator=(const BasicJobRgba &) = default;

    BasicJobRgba(BasicJobRgba &&) noexcept = default;
    BasicJobRgba &operator=(BasicJobRgba &&) noexcept = default;

    [[nodiscard]] static constexpr Channel channelMaximum() noexcept
    {
        return std::numeric_limits<Channel>::max();
    }

    [[nodiscard]] constexpr Channel red() const noexcept
    {
        return m_red;
    }

    [[nodiscard]] constexpr Channel green() const noexcept
    {
        return m_green;
    }

    [[nodiscard]] constexpr Channel blue() const noexcept
    {
        return m_blue;
    }

    [[nodiscard]] constexpr Channel alpha() const noexcept
    {
        return m_alpha;
    }

    [[nodiscard]] constexpr Channel channel(JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case JobColorChannel::Red:
            return m_red;

        case JobColorChannel::Green:
            return m_green;

        case JobColorChannel::Blue:
            return m_blue;

        case JobColorChannel::Alpha:
            return m_alpha;

        case JobColorChannel::None:
            break;
        }

        return {};
    }

    [[nodiscard]] constexpr bool opaque() const noexcept
    {
        return m_alpha == channelMaximum();
    }

    [[nodiscard]] constexpr bool transparent() const noexcept
    {
        return m_alpha == 0;
    }

    [[nodiscard]] friend constexpr bool operator==(const BasicJobRgba &, const BasicJobRgba &) noexcept = default;

private:
    Channel m_red{};
    Channel m_green{};
    Channel m_blue{};
    Channel m_alpha{channelMaximum()};
};

using JobRgba   = BasicJobRgba<std::uint8_t>;
using JobRgba16 = BasicJobRgba<std::uint16_t>;

}