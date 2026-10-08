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

#include "job_hwb.h"

namespace job::render {

class JobNcol
{
public:
    using Ptr  = std::shared_ptr<JobNcol>;
    using WPtr = std::weak_ptr<JobNcol>;
    using UPtr = std::unique_ptr<JobNcol>;

    using Channel = JobHwb::Channel;
    using Size = JobHwb::Size;

    constexpr JobNcol() noexcept = default;

    constexpr JobNcol(Channel hue, Channel whiteness, Channel blackness, Channel alpha = std::numeric_limits<Channel>::max()) noexcept
        pre(hue <= 359)
        pre(whiteness <= 100)
        pre(blackness <= 100) :
        m_hwb(hue, whiteness, blackness, alpha)
    {
    }

    ~JobNcol() = default;

    JobNcol(const JobNcol &) = default;
    JobNcol &operator=(const JobNcol &) = default;

    JobNcol(JobNcol &&) noexcept = default;
    JobNcol &operator=(JobNcol &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobNcol>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobNcol>();
    }

    [[nodiscard]] static Ptr createShared(Channel hue, Channel whiteness, Channel blackness, Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_shared<JobNcol>(hue, whiteness, blackness, alpha);
    }

    [[nodiscard]] static UPtr createUniq(Channel hue, Channel whiteness, Channel blackness,
                                         Channel alpha = std::numeric_limits<Channel>::max())
    {
        return std::make_unique<JobNcol>(hue, whiteness, blackness, alpha);
    }

    [[nodiscard]] static constexpr Channel maxChannels() noexcept
    {
        return JobHwb::maxChannels();
    }

    [[nodiscard]] static constexpr Size maxSize() noexcept
    {
        return JobHwb::maxSize();
    }

    [[nodiscard]] constexpr Channel channelMaximum(core::JobColorChannel channel) const noexcept
    {
        return m_hwb.channelMaximum(channel);
    }

    [[nodiscard]] constexpr bool hasChannel(core::JobColorChannel channel) const noexcept
    {
        return m_hwb.hasChannel(channel);
    }

    [[nodiscard]] constexpr Channel hue() const noexcept
    {
        return m_hwb.hue();
    }

    [[nodiscard]] constexpr Channel whiteness() const noexcept
    {
        return m_hwb.whiteness();
    }

    [[nodiscard]] constexpr Channel blackness() const noexcept
    {
        return m_hwb.blackness();
    }

    [[nodiscard]] constexpr Channel alpha() const noexcept
    {
        return m_hwb.alpha();
    }

    [[nodiscard]] constexpr Channel channel(core::JobColorChannel channel) const noexcept
    {
        return m_hwb.channel(channel);
    }

    [[nodiscard]] constexpr bool opaque() const noexcept
    {
        return m_hwb.opaque();
    }

    [[nodiscard]] constexpr bool transparent() const noexcept
    {
        return m_hwb.transparent();
    }

    [[nodiscard]] constexpr core::JobColorType type() const noexcept
    {
        return core::JobColorType::Ncol;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_hwb.valid();
    }

    [[nodiscard]] std::string toString() const
    {
        return "ncol(" +
               std::to_string(hue()) + "," +
               std::to_string(whiteness()) + "," +
               std::to_string(blackness()) + "," +
               std::to_string(alpha()) + ")";
    }

    [[nodiscard]] static JobNcol fromString(std::string_view value)
    {
        if (!value.starts_with("ncol(") || !value.ends_with(')'))
            return {};

        value.remove_prefix(5);
        value.remove_suffix(1);

        const auto parts = core::JobStringUtils::split(value, ',');
        if (parts.size() != 4)
            return {};

        const auto hue = core::JobStringUtils::toInteger<Channel>(parts[0]);
        const auto whiteness = core::JobStringUtils::toInteger<Channel>(parts[1]);
        const auto blackness = core::JobStringUtils::toInteger<Channel>(parts[2]);
        const auto alpha = core::JobStringUtils::toInteger<Channel>(parts[3]);

        JobNcol result(hue, whiteness, blackness, alpha);
        if (!result.valid())
            return {};

        return result;
    }

    [[nodiscard]] bool supports(core::FormatType format) const noexcept
    {
        return m_hwb.supports(format);
    }

    [[nodiscard]] friend constexpr bool operator==(const JobNcol &, const JobNcol &) noexcept = default;

private:
    JobHwb m_hwb{};
};

static_assert(core::JobColorConcept<JobNcol>);

} // namespace job::render