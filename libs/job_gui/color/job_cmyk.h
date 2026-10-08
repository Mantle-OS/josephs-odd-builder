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

class JobCmyk
{
public:
    using Ptr  = std::shared_ptr<JobCmyk>;
    using WPtr = std::weak_ptr<JobCmyk>;
    using UPtr = std::unique_ptr<JobCmyk>;

    using Channel = std::uint16_t;
    using Size = std::size_t;

    constexpr JobCmyk() noexcept = default;

    constexpr JobCmyk(Channel cyan, Channel magenta, Channel yellow, Channel black, Channel alpha = std::numeric_limits<Channel>::max()) noexcept
        pre(cyan <= 100)
        pre(magenta <= 100)
        pre(yellow <= 100)
        pre(black <= 100) :
        m_cyan(cyan),
        m_magenta(magenta),
        m_yellow(yellow),
        m_black(black),
        m_alpha(alpha)
    {
    }

    ~JobCmyk() = default;

    JobCmyk(const JobCmyk &) = default;
    JobCmyk &operator=(const JobCmyk &) = default;

    JobCmyk(JobCmyk &&) noexcept = default;
    JobCmyk &operator=(JobCmyk &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobCmyk>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobCmyk>();
    }

    [[nodiscard]] static Ptr createShared(Channel cyan, Channel magenta, Channel yellow, Channel black,
                                          Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobCmyk>(cyan, magenta, yellow, black, alpha);
    }

    [[nodiscard]] static UPtr createUniq(Channel cyan, Channel magenta, Channel yellow, Channel black,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobCmyk>(cyan, magenta, yellow, black, alpha);
    }

    [[nodiscard]] static constexpr Channel maxChannels() noexcept
    {
        return 5;
    }

    [[nodiscard]] static constexpr Size maxSize() noexcept
    {
        return sizeof(Channel) * 5;
    }

    [[nodiscard]] constexpr Channel channelMaximum(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Cyan:
        case core::JobColorChannel::Magenta:
        case core::JobColorChannel::Yellow:
        case core::JobColorChannel::Black:
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
        case core::JobColorChannel::Cyan:
        case core::JobColorChannel::Magenta:
        case core::JobColorChannel::Yellow:
        case core::JobColorChannel::Black:
        case core::JobColorChannel::Alpha:
            return true;

        default:
            return false;
        }
    }

    [[nodiscard]] constexpr Channel cyan() const noexcept
    {
        return m_cyan;
    }

    [[nodiscard]] constexpr Channel magenta() const noexcept
    {
        return m_magenta;
    }

    [[nodiscard]] constexpr Channel yellow() const noexcept
    {
        return m_yellow;
    }

    [[nodiscard]] constexpr Channel black() const noexcept
    {
        return m_black;
    }

    [[nodiscard]] constexpr Channel alpha() const noexcept
    {
        return m_alpha;
    }

    [[nodiscard]] constexpr Channel channel(core::JobColorChannel channel) const noexcept
    {
        switch (channel) {
        case core::JobColorChannel::Cyan:
            return m_cyan;

        case core::JobColorChannel::Magenta:
            return m_magenta;

        case core::JobColorChannel::Yellow:
            return m_yellow;

        case core::JobColorChannel::Black:
            return m_black;

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
        return core::JobColorType::Cmyk;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_cyan <= channelMaximum(core::JobColorChannel::Cyan) &&
               m_magenta <= channelMaximum(core::JobColorChannel::Magenta) &&
               m_yellow <= channelMaximum(core::JobColorChannel::Yellow) &&
               m_black <= channelMaximum(core::JobColorChannel::Black);
    }

    [[nodiscard]] std::string toString() const
    {
        return "cmyk(" +
               std::to_string(m_cyan) + "," +
               std::to_string(m_magenta) + "," +
               std::to_string(m_yellow) + "," +
               std::to_string(m_black) + "," +
               std::to_string(m_alpha) + ")";
    }

    [[nodiscard]] static JobCmyk fromString(std::string_view value)
    {
        if (!value.starts_with("cmyk(") || !value.ends_with(')'))
            return {};

        value.remove_prefix(5);
        value.remove_suffix(1);

        const auto parts = core::JobStringUtils::split(value, ',');
        if (parts.size() != 5)
            return {};

        const auto cyan = core::JobStringUtils::toInteger<Channel>(parts[0]);
        const auto magenta = core::JobStringUtils::toInteger<Channel>(parts[1]);
        const auto yellow = core::JobStringUtils::toInteger<Channel>(parts[2]);
        const auto black = core::JobStringUtils::toInteger<Channel>(parts[3]);
        const auto alpha = core::JobStringUtils::toInteger<Channel>(parts[4]);

        JobCmyk result(cyan, magenta, yellow, black, alpha);
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

    [[nodiscard]] friend constexpr bool operator==(const JobCmyk &, const JobCmyk &) noexcept = default;

private:
    Channel m_cyan{};
    Channel m_magenta{};
    Channel m_yellow{};
    Channel m_black{};
    Channel m_alpha{std::numeric_limits<Channel>::max()};
};

static_assert(core::JobColorConcept<JobCmyk>);

} // namespace job::render