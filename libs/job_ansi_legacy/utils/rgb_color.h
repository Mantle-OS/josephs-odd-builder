#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "job_string_utils.h"
namespace job::ansi::utils {

class RGBColor
{
public:
    constexpr RGBColor( std::uint8_t red = 0, std::uint8_t green = 255, std::uint8_t blue = 0, std::uint8_t alpha = 255) noexcept :
        m_red(red),
        m_green(green),
        m_blue(blue),
        m_alpha(alpha)
    {

    }

    ~RGBColor() = default;

    RGBColor(const RGBColor &) = default;
    RGBColor &operator=(const RGBColor &) = default;

    RGBColor(RGBColor &&) noexcept = default;
    RGBColor &operator=(RGBColor &&) noexcept = default;

    [[nodiscard]] constexpr std::uint8_t red() const noexcept
    {
        return m_red;
    }

    [[nodiscard]] constexpr std::uint8_t green() const noexcept
    {
        return m_green;
    }

    [[nodiscard]] constexpr std::uint8_t blue() const noexcept
    {
        return m_blue;
    }

    [[nodiscard]] constexpr std::uint8_t alpha() const noexcept
    {
        return m_alpha;
    }

    [[nodiscard]] constexpr std::uint32_t toARGB() const noexcept
    {
        return (static_cast<std::uint32_t>(m_alpha) << 24) |
               (static_cast<std::uint32_t>(m_red) << 16) |
               (static_cast<std::uint32_t>(m_green) << 8) |
               static_cast<std::uint32_t>(m_blue);
    }

    [[nodiscard]] constexpr std::uint32_t toRGBA() const noexcept
    {
        return (static_cast<std::uint32_t>(m_red) << 24) |
               (static_cast<std::uint32_t>(m_green) << 16) |
               (static_cast<std::uint32_t>(m_blue) << 8) |
               static_cast<std::uint32_t>(m_alpha);
    }

    [[nodiscard]] constexpr std::uint32_t toInt() const noexcept
    {
        return toRGBA();
    }

    [[nodiscard]] static constexpr RGBColor fromARGB(std::uint32_t value) noexcept
    {
        return RGBColor{
            static_cast<std::uint8_t>((value >> 16) & 0xff),
            static_cast<std::uint8_t>((value >> 8) & 0xff),
            static_cast<std::uint8_t>(value & 0xff),
            static_cast<std::uint8_t>((value >> 24) & 0xff)
        };
    }

    [[nodiscard]] static constexpr RGBColor fromRGBA(std::uint32_t value) noexcept
    {
        return RGBColor{
            static_cast<std::uint8_t>((value >> 24) & 0xff),
            static_cast<std::uint8_t>((value >> 16) & 0xff),
            static_cast<std::uint8_t>((value >> 8) & 0xff),
            static_cast<std::uint8_t>(value & 0xff)
        };
    }

    [[nodiscard]] static constexpr RGBColor fromInt(std::uint32_t value) noexcept
    {
        return fromRGBA(value);
    }

    [[nodiscard]] std::string toHexString(bool includeAlpha = false) const
    {
        // constexpr char hex[] = "0123456789ABCDEF";

        char buffer[9]{};

        buffer[0] = '#';

        buffer[1] = JobStringUtils::kHexChars[(m_red >> 4) & 0x0f];
        buffer[2] = JobStringUtils::kHexChars[m_red & 0x0f];

        buffer[3] = JobStringUtils::kHexChars[(m_green >> 4) & 0x0f];
        buffer[4] = JobStringUtils::kHexChars[m_green & 0x0f];

        buffer[5] = JobStringUtils::kHexChars[(m_blue >> 4) & 0x0f];
        buffer[6] = JobStringUtils::kHexChars[m_blue & 0x0f];

        if (!includeAlpha)
            return std::string(buffer, 7);

        buffer[7] = JobStringUtils::kHexChars[(m_alpha >> 4) & 0x0f];
        buffer[8] = JobStringUtils::kHexChars[m_alpha & 0x0f];

        return std::string(buffer, 9);
    }

    [[nodiscard]] static RGBColor fromHexString(std::string_view hex)
    {
        const auto color = tryFromHexString(hex);

        if (!color)
            throw std::invalid_argument("Hex string must be #RRGGBB or #RRGGBBAA");

        return *color;
    }

    [[nodiscard]] static constexpr std::optional<RGBColor> tryFromHexString(std::string_view hex) noexcept
    {
        if (hex.size() != 7 && hex.size() != 9)
            return std::nullopt;

        if (hex[0] != '#')
            return std::nullopt;

        const auto red = parseByte(hex[1], hex[2]);
        const auto green = parseByte(hex[3], hex[4]);
        const auto blue = parseByte(hex[5], hex[6]);

        if (!red || !green || !blue)
            return std::nullopt;

        std::uint8_t alpha = 255;

        if (hex.size() == 9) {
            const auto parsedAlpha = parseByte(hex[7], hex[8]);

            if (!parsedAlpha)
                return std::nullopt;

            alpha = *parsedAlpha;
        }

        return RGBColor{
            *red,
            *green,
            *blue,
            alpha
        };
    }

    [[nodiscard]] friend constexpr bool operator==(const RGBColor &, const RGBColor &) noexcept = default;

private:
    [[nodiscard]] static constexpr std::optional<std::uint8_t> hexValue(char value) noexcept
    {
        if (value >= '0' && value <= '9')
            return static_cast<std::uint8_t>(value - '0');

        if (value >= 'a' && value <= 'f')
            return static_cast<std::uint8_t>(value - 'a' + 10);

        if (value >= 'A' && value <= 'F')
            return static_cast<std::uint8_t>(value - 'A' + 10);

        return std::nullopt;
    }

    [[nodiscard]] static constexpr std::optional<std::uint8_t> parseByte(char high, char low) noexcept
    {
        const auto highValue = hexValue(high);
        const auto lowValue = hexValue(low);

        if (!highValue || !lowValue)
            return std::nullopt;

        return static_cast<std::uint8_t>((*highValue << 4) | *lowValue);
    }

    std::uint8_t m_red{};
    std::uint8_t m_green{255};
    std::uint8_t m_blue{};
    std::uint8_t m_alpha{255};
};

[[nodiscard]] inline constexpr std::optional<RGBColor> parseColorString(std::string_view input) noexcept
{
    return RGBColor::tryFromHexString(input);
}

}