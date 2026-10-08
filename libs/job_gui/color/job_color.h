#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <meta>

#include <job_color_channel.h>
#include <job_string_utils.h>

#include "job_color_keywords.h"
#include "job_color_ansi.h"

#include "job_rgba.h"
#include "job_rgba_16.h"
#include "job_hsv.h"
#include "job_hsl.h"
#include "job_cmyk.h"
#include "job_hwb.h"
#include "job_ncol.h"

namespace job::render {

class JobColor
{
public:
    using ColorVariant = std::variant<JobRgba, JobRgba16, JobHsv, JobHsl, JobCmyk, JobHwb, JobNcol>;
    // using ChannelValue = std::variant<std::uint8_t, std::uint16_t>;
    // using SizeValue = std::variant<
    //     JobRgba::Size,
    //     JobRgba16::Size,
    //     JobHsv::Size,
    //     JobHsl::Size,
    //     JobCmyk::Size,
    //     JobHwb::Size,
    //     JobNcol::Size
    //     >;
    JobColor() :
    {
    }


    JobColor(core::JobColorType type = core::JobColorType::Rgba) :
    {
        switch (type) {
            case core::JobColorType::Rgba:
                m_color = JobRgba{};
                break;
            case core::JobColorType::Rgba16:
                m_color = JobRgba16{};
                break;
            case core::JobColorType::Hsv:
                m_color = JobHsv{};
                break;
            case core::JobColorType::Hsl:
                m_color = JobHsl{};
                break;
            case core::JobColorType::Cmyk:
                m_color = JobCmyk{};
                break;
            case core::JobColorType::NCol:
                m_color = JobNcol{};
                break;
            case core::JobColorType::Hwb:
                m_color = JobHwb{};
                break;
            case core::JobColorType::Invalid:
                m_color = JobRgba{}; // your stuck with this :P
                break;
        }
    }

    template<core::JobColorConcept Color>
    JobColor(Color color, std::string_view color_name) noexcept :
        m_color{color}
    {
        m_color.fromString(color_name);
    }


    JobColor(core::JobColorType type = core::JobColorType::Rgba, const std::string_view color_name) noexcept :
    {
        switch (type) {
        case core::JobColorType::Rgba:
            m_color = JobRgba{color_name};
            break;
        case core::JobColorType::Rgba16:
            m_color = JobRgba16{color_name};
            break;
        case core::JobColorType::Hsv:
            m_color = JobHsv::fromString{color_name};
            break;
        case core::JobColorType::Hsl:
            m_color = JobHsl::fromString{color_name};
            break;
        case core::JobColorType::Cmyk:
            m_color = JobCmyk::fromString{color_name};
            break;
        case core::JobColorType::NCol:
            m_color = JobNcol::fromString{color_name};
            break;
        case core::JobColorType::Hwb:
            m_color = JobHwb::fromString{color_name};
            break;
        case core::JobColorType::Invalid:
            m_color = JobRgba{color_name}; // your stuck with this :P
            break;
        }
    }

    JobColor(JobRgba rgba) noexcept :
        m_color{rgba}
    {

    }

    JobColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) noexcept :
        m_color{ JobRgba{red, green, blue, alpha}}
    {         
    }

    JobColor(JobRgba16 rgba) noexcept :
        m_color{rgba}
    {

    }
    JobColor(std::uint16_t red, std::uint16_t green, std::uint16_t blue, std::uint16_t alpha = 255) noexcept :
        m_color{ JobRgba16{red, green, blue, alpha}}
    {
    }

    JobColor(JobHsv hsv) noexcept :
        m_color{hsv}
    {

    }
    void fromHsv(std::uint16_t hue, std::uint16_t saturation, std::uint16_t value, std::uint16_t alpha = std::numeric_limits<std::uint16_t>::max())
    {
        JobHsv hsv{hue, saturation, value, alpha};
        setColor(hsv);
    }

    JobColor(JobHsl hsl) noexcept :
        m_color{hsl}
    {

    }
    void fromHsl(std::uint16_t hue, std::uint16_t saturation, std::uint16_t lightness, std::uint16_t alpha = std::numeric_limits<std::uint16_t>::max())
    {
        JobHsl hsl{hue, saturation, lightness, alpha};
        setColor(hsl);
    }


    JobColor(JobCmyk cmyk) noexcept :
        m_color{cmyk}
    {
    }
    void fromCmyk(std::uint16_t cyan, std::uint16_t magenta, std::uint16_t yellow, std::uint16_t black,  std::uint16_t alpha = std::numeric_limits<std::uint16_t>::max())
    {
        JobCmyk cmyk{cyan, magenta, yellow, black, alpha};
        setColor(cmyk);
    }


    JobColor(JobNcol ncol) noexcept :
        m_color{ncol}
    {
    }
    void fromNcol(std::uint16_t hue, std::uint16_t whiteness, std::uint16_t blackness, std::uint16_t alpha = std::numeric_limits<std::uint16_t>::max())
    {
        JobNcol ncol{hue, whiteness, blackness, alpha};
        setColor(ncol);
    }

    JobColor(JobHwb hwb) noexcept :
        m_color{hwb}
    {
    }
    void fromHwb(std::uint16_t hue, std::uint16_t whiteness, std::uint16_t blackness, std::uint16_t alpha = std::numeric_limits<std::uint16_t>::max())
    {
        JobHwb hwb{hue, whiteness, blackness, alpha};
        setColor(hwb);
    }

    ~JobColor() = default;

    JobColor(const JobColor &) = default;
    JobColor &operator=(const JobColor &) = default;

    JobColor(JobColor &&) noexcept = default;
    JobColor &operator=(JobColor &&) noexcept = default;

    template <core::JobColorConcept Color>
    [[nodiscard]] bool is() const noexcept
    {
        return std::holds_alternative<Color>(m_color);
    }

    template <core::JobColorConcept Color>
    [[nodiscard]] const Color *color() const noexcept
    {
        return std::get_if<Color>(&m_color);
    }

    template <core::JobColorConcept Color>
    [[nodiscard]] Color *color() noexcept
    {
        return std::get_if<Color>(&m_color);
    }

    template <core::JobColorConcept Color>
    void setColor(Color color) noexcept
    {
        m_color = std::move(color);
    }

    [[nodiscard]] std::meta::info maxChannelsType() const noexcept
    {
        return std::visit([]<typename Color>(const Color &) {
            return ^^Color::Channel;
        }, m_color);
    }

    template <typename T>
    [[nodiscard]] bool maxChannels(T &value) const noexcept
    {
        return std::visit([&value]<typename Color>(const Color &) {
            using Channel = typename Color::Channel;

            if constexpr (std::meta::is_same_type(^^T, ^^Channel)) {
                value = Color::maxChannels();
                return true;
            }

            return false;
        }, m_color);
    }

    [[nodiscard]] std::meta::info maxSizeType() const noexcept
    {
        return std::visit([]<typename Color>(const Color &) {
            return ^^Color::Size;
        }, m_color);
    }

    template <typename T>
    [[nodiscard]] bool maxSize(T &value) const noexcept
    {
        return std::visit([&value]<typename Color>(const Color &) {
            using Size = typename Color::Size;

            if constexpr (std::meta::is_same_type(^^T, ^^Size)) {
                value = Color::maxSize();
                return true;
            }

            return false;
        }, m_color);
    }

    [[nodiscard]] bool hasChannel(core::JobColorChannel channel) const noexcept
    {
        return std::visit([channel](const auto &color) {
            return color.hasChannel(channel);
        }, m_color);
    }

    [[nodiscard]] std::meta::info channelType() const noexcept
    {
        return std::visit([]<typename Color>(const Color &) {
            return ^^Color::Channel;
        }, m_color);
    }

    template <typename T>
    [[nodiscard]] bool channel(core::JobColorChannel channel, T &value) const noexcept
    {
        return std::visit([channel, &value]<typename Color>(const Color &color) {
            using Channel = typename Color::Channel;

            if constexpr (std::meta::is_same_type(^^T, ^^Channel)) {
                value = color.channel(channel);
                return true;
            }

            return false;
        }, m_color);
    }

    [[nodiscard]] std::meta::info channelMaximumType() const noexcept
    {
        return std::visit([]<typename Color>(const Color &) {
            return ^^Color::Channel;
        }, m_color);
    }

    template <typename T>
    [[nodiscard]] bool channelMaximum(core::JobColorChannel channel, T &value) const noexcept
    {
        return std::visit([channel, &value]<typename Color>(const Color &color) {
            using Channel = typename Color::Channel;

            if constexpr (std::meta::is_same_type(^^T, ^^Channel)) {
                value = color.channelMaximum(channel);
                return true;
            }

            return false;
        }, m_color);
    }

    [[nodiscard]] std::meta::info alphaType() const noexcept
    {
        return std::visit([]<typename Color>(const Color &) {
            return ^^Color::Channel;
        }, m_color);
    }

    template <typename T>
    [[nodiscard]] bool alpha(T &value) const noexcept
    {
        return std::visit([&value]<typename Color>(const Color &color) {
            using Channel = typename Color::Channel;

            if constexpr (std::meta::is_same_type(^^T, ^^Channel)) {
                value = color.alpha();
                return true;
            }

            return false;
        }, m_color);
    }

    [[nodiscard]] bool opaque() const noexcept
    {
        return std::visit([](const auto &color) {
            return color.opaque();
        }, m_color);
    }

    [[nodiscard]] bool transparent() const noexcept
    {
        return std::visit([](const auto &color) {
            return color.transparent();
        }, m_color);
    }

    [[nodiscard]] core::JobColorType type() const noexcept
    {
        return std::visit([](const auto &color) {
            return color.type();
        }, m_color);
    }

    [[nodiscard]] std::string toString() const
    {
        return std::visit([](const auto &color) {
            return color.toString();
        }, m_color);
    }

    [[nodiscard]] bool supports(core::FormatType format) const noexcept
    {
        return std::visit([format](const auto &color) {
            return color.supports(format);
        }, m_color);
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return std::visit([](const auto &color) {
            return color.valid();
        }, m_color);
    }
private:
    ColorVariant m_color{JobRgba{}};
};

}