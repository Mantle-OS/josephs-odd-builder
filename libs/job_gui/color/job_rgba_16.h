#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include <job_string_utils.h>
#include <job_color_channel.h>
#include <job_color_concept.h>
#include <job_color_type.h>
#include <job_render_format.h>

#include "job_color_string.h"

namespace job::render {

class JobRgba16
{
public:
    using Ptr     = std::shared_ptr<JobRgba16>;
    using WPtr    = std::weak_ptr<JobRgba16>;
    using UPtr    = std::unique_ptr<JobRgba16>;
    using Channel = std::uint16_t;
    using Size    = std::size_t;

    constexpr JobRgba16() noexcept = default;

    explicit JobRgba16(std::string_view value)
    {
        const auto [type, packed] = gui::JobColorString::parseColor(value);
        if (type != core::JobColorType::Rgba16)
            return;

        m_alpha = static_cast<Channel>((packed >> 48) & 0xFFFF);
        m_red   = static_cast<Channel>((packed >> 32) & 0xFFFF);
        m_green = static_cast<Channel>((packed >> 16) & 0xFFFF);
        m_blue  = static_cast<Channel>(packed & 0xFFFF);
    }

    constexpr JobRgba16(Channel red, Channel green, Channel blue,
                        Channel alpha = std::numeric_limits<Channel>::max()) noexcept :
        m_red(red),
        m_green(green),
        m_blue(blue),
        m_alpha(alpha)
    {
    }

    ~JobRgba16() = default;

    JobRgba16(const JobRgba16 &) = default;
    JobRgba16 &operator=(const JobRgba16 &) = default;

    JobRgba16(JobRgba16 &&) noexcept = default;
    JobRgba16 &operator=(JobRgba16 &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobRgba16>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobRgba16>();
    }

    [[nodiscard]] static Ptr createShared(std::string_view value)
    {
        return std::make_shared<JobRgba16>(value);
    }

    [[nodiscard]] static Ptr createShared(Channel red, Channel green, Channel blue,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobRgba16>(red, green, blue, alpha);
    }

    [[nodiscard]] static UPtr createUniq(std::string_view value)
    {
        return std::make_unique<JobRgba16>(value);
    }

    [[nodiscard]] static UPtr createUniq(Channel red, Channel green, Channel blue,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobRgba16>(red, green, blue, alpha);
    }

    [[nodiscard]] static constexpr Channel maxChannels() noexcept
    {
        return 4;
    }

    [[nodiscard]] static constexpr Size maxSize() noexcept
    {
        return sizeof(Channel) * 4;
    }

    [[nodiscard]] static constexpr Channel channelMaximum() noexcept
    {
        return std::numeric_limits<Channel>::max();
    }

    [[nodiscard]] constexpr Channel channelMaximum(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Red:
        case core::JobColorChannel::Green:
        case core::JobColorChannel::Blue:
        case core::JobColorChannel::Alpha:
            return channelMaximum();

        default:
            return {};
        }
    }

    [[nodiscard]] constexpr bool hasChannel(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Red:
        case core::JobColorChannel::Green:
        case core::JobColorChannel::Blue:
        case core::JobColorChannel::Alpha:
            return true;

        default:
            return false;
        }
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

    [[nodiscard]] constexpr Channel channel(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Red:
            return m_red;

        case core::JobColorChannel::Green:
            return m_green;

        case core::JobColorChannel::Blue:
            return m_blue;

        case core::JobColorChannel::Alpha:
            return m_alpha;

        default:
            return {};
        }
    }

    [[nodiscard]] constexpr bool opaque() const noexcept
    {
        return m_alpha == channelMaximum(core::JobColorChannel::Alpha);
    }

    [[nodiscard]] constexpr bool transparent() const noexcept
    {
        return m_alpha == 0;
    }

    [[nodiscard]] constexpr core::JobColorType type() const noexcept
    {
        return core::JobColorType::Rgba16;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return true;
    }

    [[nodiscard]] std::string toString() const
    {
        std::string out(17, '#');
        const auto writeChannel = [&](std::size_t offset, Channel value) {
            out[offset + 0] = core::JobStringUtils::kHexChars[(value >> 12) & 0x0F];
            out[offset + 1] = core::JobStringUtils::kHexChars[(value >> 8) & 0x0F];
            out[offset + 2] = core::JobStringUtils::kHexChars[(value >> 4) & 0x0F];
            out[offset + 3] = core::JobStringUtils::kHexChars[value & 0x0F];
        };

        writeChannel(1, m_alpha);
        writeChannel(5, m_red);
        writeChannel(9, m_green);
        writeChannel(13, m_blue);

        return out;
    }

    [[nodiscard]] static JobRgba16 fromString(std::string_view value)
    {
        return JobRgba16(value);
    }

    [[nodiscard]] bool supports(core::FormatType format) const noexcept
    {
        const auto info = core::JobRenderFormat::info(format);

        for (const auto &channel : info.channels) {
            if (!channel.valid())
                continue;

            if (!hasChannel(channel.channel))
                return false;
        }

        return true;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobRgba16 &, const JobRgba16 &) noexcept = default;

private:
    Channel m_red{};
    Channel m_green{};
    Channel m_blue{};
    Channel m_alpha{channelMaximum()};
};

static_assert(core::JobColorConcept<JobRgba16>);

} // namespace job::render