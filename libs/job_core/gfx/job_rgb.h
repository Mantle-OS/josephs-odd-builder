#pragma once

#include <cstdint>

#include "job_string_utils.h"

namespace job_::core {

struct JobRgb
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a{255};

    [[nodiscard]] friend constexpr bool operator==(const JobRgb &, const JobRgb &) noexcept = default;
    [[nodiscard]] friend constexpr bool operator!=(const JobRgb &, const JobRgb &) noexcept = default;

    [[nodiscard]] constexpr std::uint64_t toInt() const noexcept
    {
        return (static_cast<std::uint64_t>(a) << 24) |
               (static_cast<std::uint64_t>(r) << 16) |
               (static_cast<std::uint64_t>(g) << 8) |
               static_cast<std::uint64_t>(b);
    }

    [[nodiscard]] static constexpr core::JobRgb lighter(core::JobRgb color, int percent) noexcept
    {
        percent = std::clamp(percent, 0, 256);
        const int red = static_cast<int>(color.r) + ((255 - static_cast<int>(color.r)) * percent / 256);
        const int green = static_cast<int>(color.g) + ((255 - static_cast<int>(color.g)) * percent / 256);
        const int blue = static_cast<int>(color.b) + ((255 - static_cast<int>(color.b)) * percent / 256);

        return core::JobRgb{
            static_cast<std::uint8_t>(red),
            static_cast<std::uint8_t>(green),
            static_cast<std::uint8_t>(blue),
            color.a
        };
    }

    [[nodiscard]] static constexpr core::JobRgb darker(core::JobRgb color, int percent) noexcept
    {
        percent = std::clamp(percent, 0, 256);

        const int scale = 256 - percent;

        const int red = static_cast<int>(color.r) * scale / 256;
        const int green = static_cast<int>(color.g) * scale / 256;
        const int blue = static_cast<int>(color.b) * scale / 256;

        return core::JobRgb{
            static_cast<std::uint8_t>(red),
            static_cast<std::uint8_t>(green),
            static_cast<std::uint8_t>(blue),
            color.a
        };
    }


    [[nodiscard]] constexpr std::uint32_t toARGB() const noexcept
    {
        return (static_cast<std::uint32_t>(a) << 24) |
               (static_cast<std::uint32_t>(r) << 16) |
               (static_cast<std::uint32_t>(g) << 8) |
               static_cast<std::uint32_t>(b);
    }

    [[nodiscard]] constexpr std::uint32_t toRGBA() const noexcept
    {
        return (static_cast<std::uint32_t>(r) << 24) |
               (static_cast<std::uint32_t>(g) << 16) |
               (static_cast<std::uint32_t>(b) << 8) |
               static_cast<std::uint32_t>(a);
    }

    [[nodiscard]] constexpr std::uint32_t toInt() const noexcept
    {
        return toRGBA();
    }

    [[nodiscard]] static constexpr JobRgb fromARGB(std::uint32_t value) noexcept
    {
        return JobRgb{
            static_cast<std::uint8_t>((value >> 16) & 0xff),
            static_cast<std::uint8_t>((value >> 8) & 0xff),
            static_cast<std::uint8_t>(value & 0xff),
            static_cast<std::uint8_t>((value >> 24) & 0xff)
        };
    }

    [[nodiscard]] static constexpr JobRgb fromRGBA(std::uint32_t value) noexcept
    {
        return JobRgb{
            static_cast<std::uint8_t>((value >> 24) & 0xff),
            static_cast<std::uint8_t>((value >> 16) & 0xff),
            static_cast<std::uint8_t>((value >> 8) & 0xff),
            static_cast<std::uint8_t>(value & 0xff)
        };
    }

    [[nodiscard]] static constexpr JobRgb fromInt(std::uint32_t value) noexcept
    {
        return fromRGBA(value);
    }

    [[nodiscard]] std::string toHexString(bool includeAlpha = false) const
    {
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

    // this will all get updated later on somewhere else. For now it can live here
    [[nodiscard]] static JobRgb fromHexString(std::string_view hex)
    {
        const auto color = tryFromHexString(hex);

        if (!color)
            throw std::invalid_argument("Hex string must be #RRGGBB or #RRGGBBAA");

        return *color;
    }

    // this will all get updated later on somewhere else. For now it can live here
    [[nodiscard]] static constexpr std::optional<JobRgb> tryFromHexString(std::string_view hex) noexcept
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

        return JobRgb{
            *red,
            *green,
            *blue,
            alpha
        };
    }

    // this will all get updated later on somewhere else. For now it can live here
    [[nodiscard]] static constexpr std::optional<JobRgb> parseColorString(std::string_view input) noexcept
    {
        return tryFromHexString(input);
    }

private:
    // this will all get updated later on somewhere else. For now it can live here
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

    // this will all get updated later on somewhere else. For now it can live here
    [[nodiscard]] static constexpr std::optional<std::uint8_t> parseByte(char high, char low) noexcept
    {
        const auto highValue = hexValue(high);
        const auto lowValue = hexValue(low);

        if (!highValue || !lowValue)
            return std::nullopt;

        return static_cast<std::uint8_t>((*highValue << 4) | *lowValue);
    }

};

}