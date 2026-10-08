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

class JobHsv
{
public:
    using Ptr  = std::shared_ptr<JobHsv>;
    using WPtr = std::weak_ptr<JobHsv>;
    using UPtr = std::unique_ptr<JobHsv>;

    using Channel = std::uint16_t;
    using Size = std::size_t;

    constexpr JobHsv() noexcept = default;

    constexpr JobHsv(Channel hue, Channel saturation, Channel value,
                     Channel alpha = std::numeric_limits<Channel>::max()) noexcept :
        m_hue(hue),
        m_saturation(saturation),
        m_value(value),
        m_alpha(alpha)
    {
    }

    ~JobHsv() = default;

    JobHsv(const JobHsv &) = default;
    JobHsv &operator=(const JobHsv &) = default;

    JobHsv(JobHsv &&) noexcept = default;
    JobHsv &operator=(JobHsv &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobHsv>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobHsv>();
    }

    [[nodiscard]] static Ptr createShared(Channel hue, Channel saturation, Channel value,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobHsv>(hue, saturation, value, alpha);
    }

    [[nodiscard]] static UPtr createUniq(Channel hue, Channel saturation, Channel value,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobHsv>(hue, saturation, value, alpha);
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
        case core::JobColorChannel::Value:
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
        case core::JobColorChannel::Value:
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

    [[nodiscard]] constexpr Channel value() const noexcept
    {
        return m_value;
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

        case core::JobColorChannel::Value:
            return m_value;

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
        return core::JobColorType::Hsv;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_hue <= channelMaximum(core::JobColorChannel::Hue) &&
               m_saturation <= channelMaximum(core::JobColorChannel::Saturation) &&
               m_value <= channelMaximum(core::JobColorChannel::Value);
    }

    [[nodiscard]] std::string toString() const
    {
        return "hsv(" +
               std::to_string(m_hue) + "," +
               std::to_string(m_saturation) + "," +
               std::to_string(m_value) + "," +
               std::to_string(m_alpha) + ")";
    }

    [[nodiscard]] static JobHsv fromString(std::string_view value)
    {
        if (!value.starts_with("hsv(") || !value.ends_with(')'))
            return {};

        value.remove_prefix(4);
        value.remove_suffix(1);

        const auto parts = core::JobStringUtils::split(value, ',');
        if (parts.size() != 4)
            return {};

        const auto hue = core::JobStringUtils::toInteger<Channel>(parts[0]);
        const auto saturation = core::JobStringUtils::toInteger<Channel>(parts[1]);
        const auto val = core::JobStringUtils::toInteger<Channel>(parts[2]);
        const auto alpha = core::JobStringUtils::toInteger<Channel>(parts[3]);

        JobHsv result(hue, saturation, val, alpha);
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

    [[nodiscard]] friend constexpr bool operator==(const JobHsv &, const JobHsv &) noexcept = default;

private:
    Channel m_hue{};
    Channel m_saturation{};
    Channel m_value{};
    Channel m_alpha{std::numeric_limits<Channel>::max()};
};

static_assert(core::JobColorConcept<JobHsv>);

} // namespace job::render