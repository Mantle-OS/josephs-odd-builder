#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include <job_color_channel.h>
#include <job_color_concept.h>
#include <job_color_type.h>
#include <job_render_format.h>
#include <job_string_utils.h>

namespace job::render {

class JobHwb
{
public:
    using Ptr  = std::shared_ptr<JobHwb>;
    using WPtr = std::weak_ptr<JobHwb>;
    using UPtr = std::unique_ptr<JobHwb>;

    using Channel = std::uint16_t;
    using Size = std::size_t;

    constexpr JobHwb() noexcept = default;

    constexpr JobHwb(Channel hue, Channel whiteness, Channel blackness, Channel alpha = std::numeric_limits<Channel>::max()) noexcept
        pre(hue <= 359)
        pre(whiteness <= 100)
        pre(blackness <= 100) :
        m_hue(hue),
        m_whiteness(whiteness),
        m_blackness(blackness),
        m_alpha(alpha)
    {
    }

    ~JobHwb() = default;

    JobHwb(const JobHwb &) = default;
    JobHwb &operator=(const JobHwb &) = default;

    JobHwb(JobHwb &&) noexcept = default;
    JobHwb &operator=(JobHwb &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHwb>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHwb>();
    }

    [[nodiscard]] static Ptr createShared(Channel hue, Channel whiteness, Channel blackness,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobHwb>(hue, whiteness, blackness, alpha);
    }

    [[nodiscard]] static UPtr createUniq(Channel hue, Channel whiteness, Channel blackness,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobHwb>(hue, whiteness, blackness, alpha);
    }

    [[nodiscard]] static constexpr Channel maxChannels() noexcept
    {
        return 4;
    }

    [[nodiscard]] static constexpr Size maxSize() noexcept
    {
        return sizeof(Channel) * 4;
    }

    [[nodiscard]] constexpr Channel channelMaximum(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Hue:
            return 359;

        case core::JobColorChannel::Whiteness:
        case core::JobColorChannel::Blackness:
            return 100;

        case core::JobColorChannel::Alpha:
            return std::numeric_limits<Channel>::max();

        default:
            return {};
        }
    }

    [[nodiscard]] constexpr bool hasChannel(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Hue:
        case core::JobColorChannel::Whiteness:
        case core::JobColorChannel::Blackness:
        case core::JobColorChannel::Alpha:
            return true;

        default:
            return false;
        }
    }

    [[nodiscard]] constexpr Channel hue() const noexcept
    {
        return m_hue;
    }

    [[nodiscard]] constexpr Channel whiteness() const noexcept
    {
        return m_whiteness;
    }

    [[nodiscard]] constexpr Channel blackness() const noexcept
    {
        return m_blackness;
    }

    [[nodiscard]] constexpr Channel alpha() const noexcept
    {
        return m_alpha;
    }

    [[nodiscard]] constexpr Channel channel(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Hue:
            return m_hue;

        case core::JobColorChannel::Whiteness:
            return m_whiteness;

        case core::JobColorChannel::Blackness:
            return m_blackness;

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
        return core::JobColorType::Hwb;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_hue <= channelMaximum(core::JobColorChannel::Hue) &&
               m_whiteness <= channelMaximum(core::JobColorChannel::Whiteness) &&
               m_blackness <= channelMaximum(core::JobColorChannel::Blackness);
    }

    [[nodiscard]] std::string toString() const
    {
        return "hwb(" +
               std::to_string(m_hue) + "," +
               std::to_string(m_whiteness) + "," +
               std::to_string(m_blackness) + "," +
               std::to_string(m_alpha) + ")";
    }

    [[nodiscard]] static JobHwb fromString(std::string_view value)
    {
        if (!value.starts_with("hwb(") || !value.ends_with(')'))
            return {};

        value.remove_prefix(4);
        value.remove_suffix(1);

        const auto parts = core::JobStringUtils::split(value, ',');
        if (parts.size() != 4)
            return {};

        const auto hue = core::JobStringUtils::toInteger<Channel>(parts[0]);
        const auto whiteness = core::JobStringUtils::toInteger<Channel>(parts[1]);
        const auto blackness = core::JobStringUtils::toInteger<Channel>(parts[2]);
        const auto alpha = core::JobStringUtils::toInteger<Channel>(parts[3]);

        JobHwb result(hue, whiteness, blackness, alpha);
        if (!result.valid())
            return {};

        return result;
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

    [[nodiscard]] friend constexpr bool operator==(const JobHwb &, const JobHwb &) noexcept = default;

private:
    Channel m_hue{};
    Channel m_whiteness{};
    Channel m_blackness{};
    Channel m_alpha{std::numeric_limits<Channel>::max()};
};

static_assert(core::JobColorConcept<JobHwb>);

} // namespace job::render