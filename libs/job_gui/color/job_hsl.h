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

class JobHsl
{
public:
    using Ptr  = std::shared_ptr<JobHsl>;
    using WPtr = std::weak_ptr<JobHsl>;
    using UPtr = std::unique_ptr<JobHsl>;

    using Channel = std::uint16_t;
    using Size = std::size_t;

    constexpr JobHsl() noexcept = default;

    constexpr JobHsl(Channel hue, Channel saturation, Channel lightness,
                     Channel alpha = std::numeric_limits<Channel>::max()) noexcept
        pre(hue <= 359)
        pre(saturation <= 100)
        pre(lightness <= 100) :
        m_hue(hue),
        m_saturation(saturation),
        m_lightness(lightness),
        m_alpha(alpha)
    {
    }

    ~JobHsl() = default;

    JobHsl(const JobHsl &) = default;
    JobHsl &operator=(const JobHsl &) = default;

    JobHsl(JobHsl &&) noexcept = default;
    JobHsl &operator=(JobHsl &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHsl>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHsl>();
    }

    [[nodiscard]] static Ptr createShared(Channel hue, Channel saturation, Channel lightness,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobHsl>(hue, saturation, lightness, alpha);
    }

    [[nodiscard]] static UPtr createUniq(Channel hue, Channel saturation, Channel lightness,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobHsl>(hue, saturation, lightness, alpha);
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

        case core::JobColorChannel::Saturation:
        case core::JobColorChannel::Lightness:
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
        case core::JobColorChannel::Saturation:
        case core::JobColorChannel::Lightness:
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

    [[nodiscard]] constexpr Channel saturation() const noexcept
    {
        return m_saturation;
    }

    [[nodiscard]] constexpr Channel lightness() const noexcept
    {
        return m_lightness;
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

        case core::JobColorChannel::Saturation:
            return m_saturation;

        case core::JobColorChannel::Lightness:
            return m_lightness;

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
        return core::JobColorType::Hsl;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_hue <= channelMaximum(core::JobColorChannel::Hue) &&
               m_saturation <= channelMaximum(core::JobColorChannel::Saturation) &&
               m_lightness <= channelMaximum(core::JobColorChannel::Lightness);
    }

    [[nodiscard]] std::string toString() const
    {
        return "hsl(" +
               std::to_string(m_hue) + "," +
               std::to_string(m_saturation) + "," +
               std::to_string(m_lightness) + "," +
               std::to_string(m_alpha) + ")";
    }

    [[nodiscard]] static JobHsl fromString(std::string_view value)
    {
        if (!value.starts_with("hsl(") || !value.ends_with(')'))
            return {};

        value.remove_prefix(4);
        value.remove_suffix(1);

        const auto parts = core::JobStringUtils::split(value, ',');
        if (parts.size() != 4)
            return {};

        const auto hue = core::JobStringUtils::toInteger<Channel>(parts[0]);
        const auto saturation = core::JobStringUtils::toInteger<Channel>(parts[1]);
        const auto lightness = core::JobStringUtils::toInteger<Channel>(parts[2]);
        const auto alpha = core::JobStringUtils::toInteger<Channel>(parts[3]);

        JobHsl result(hue, saturation, lightness, alpha);
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

    [[nodiscard]] friend constexpr bool operator==(const JobHsl &, const JobHsl &) noexcept = default;

private:
    Channel m_hue{};
    Channel m_saturation{};
    Channel m_lightness{};
    Channel m_alpha{std::numeric_limits<Channel>::max()};
};

static_assert(core::JobColorConcept<JobHsl>);

} // namespace job::render