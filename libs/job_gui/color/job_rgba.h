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

class JobRgba
{
public:
    using Ptr  = std::shared_ptr<JobRgba>;
    using WPtr = std::weak_ptr<JobRgba>;
    using UPtr = std::unique_ptr<JobRgba>;

    using Channel = std::uint8_t;
    using Size = std::size_t;

    constexpr JobRgba() noexcept = default;

    explicit JobRgba(std::string_view value)
    {
        const auto [type, packed] = gui::JobColorString::parseColor(value);
        if (type != core::JobColorType::Rgba)
            return;

        m_alpha = static_cast<Channel>((packed >> 24) & 0xFF);
        m_red   = static_cast<Channel>((packed >> 16) & 0xFF);
        m_green = static_cast<Channel>((packed >> 8) & 0xFF);
        m_blue  = static_cast<Channel>(packed & 0xFF);
    }

    constexpr JobRgba(Channel red, Channel green, Channel blue,
                      Channel alpha = std::numeric_limits<Channel>::max()) noexcept :
        m_red(red),
        m_green(green),
        m_blue(blue),
        m_alpha(alpha)
    {
    }

    ~JobRgba() = default;

    JobRgba(const JobRgba &) = default;
    JobRgba &operator=(const JobRgba &) = default;

    JobRgba(JobRgba &&) noexcept = default;
    JobRgba &operator=(JobRgba &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobRgba>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobRgba>();
    }

    [[nodiscard]] static Ptr createShared(std::string_view value)
    {
        return std::make_shared<JobRgba>(value);
    }

    [[nodiscard]] static Ptr createShared(Channel red, Channel green, Channel blue,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobRgba>(red, green, blue, alpha);
    }

    [[nodiscard]] static UPtr createUniq(std::string_view value)
    {
        return std::make_unique<JobRgba>(value);
    }

    [[nodiscard]] static UPtr createUniq(Channel red, Channel green, Channel blue,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobRgba>(red, green, blue, alpha);
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
        return core::JobColorType::Rgba;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return true;
    }

    [[nodiscard]] std::string toString() const
    {
        std::string out(9, '#');

        out[1] = core::JobStringUtils::kHexChars[(m_alpha >> 4) & 0x0F];
        out[2] = core::JobStringUtils::kHexChars[m_alpha & 0x0F];

        out[3] = core::JobStringUtils::kHexChars[(m_red >> 4) & 0x0F];
        out[4] = core::JobStringUtils::kHexChars[m_red & 0x0F];

        out[5] = core::JobStringUtils::kHexChars[(m_green >> 4) & 0x0F];
        out[6] = core::JobStringUtils::kHexChars[m_green & 0x0F];

        out[7] = core::JobStringUtils::kHexChars[(m_blue >> 4) & 0x0F];
        out[8] = core::JobStringUtils::kHexChars[m_blue & 0x0F];

        return out;
    }

    [[nodiscard]] static JobRgba fromString(std::string_view value)
    {
        return JobRgba(value);
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

    [[nodiscard]] friend constexpr bool operator==(const JobRgba &, const JobRgba &) noexcept = default;

private:
    Channel m_red{};
    Channel m_green{};
    Channel m_blue{};
    Channel m_alpha{channelMaximum()};
};

static_assert(core::JobColorConcept<JobRgba>);

} // namespace job::render