#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <charconv>

namespace job::ansi::utils {


/**
 * @class RGBColor
 * @brief Represents an RGBA color for terminal cell foreground/background.
 *
 * Used by Attributes to store color information. Supports conversion to ARGB and basic color operations.
 */
class RGBColor {
private:
    uint8_t r = 0;
    uint8_t g = 255;
    uint8_t b = 0;
    uint8_t a = 255; // Alpha (optional, default fully opaque)

public:
    constexpr RGBColor(uint8_t red = 0, uint8_t green = 255, uint8_t blue = 0, uint8_t alpha = 255):
        r(red),
        g(green),
        b(blue),
        a(alpha)
    {

    }

    constexpr bool operator==(const RGBColor &other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    constexpr bool operator!=(const RGBColor &other) const
    {
        return !(*this == other);
    }

    constexpr uint8_t red() const
    {
        return r;
    }

    constexpr uint8_t green() const
    {
        return g;
    }

    constexpr uint8_t blue() const
    {
        return b;
    }

    constexpr uint8_t alpha() const
    {
        return a;
    }


    // Convert to 0xAARRGGBB
    constexpr uint32_t toARGB() const
    {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(b);
    }

    // Convert to 0xRRGGBBAA
    constexpr uint32_t toRGBA() const
    {
        return (static_cast<uint32_t>(r) << 24) |
               (static_cast<uint32_t>(g) << 16) |
               (static_cast<uint32_t>(b) << 8)  |
               static_cast<uint32_t>(a);
    }

    // Convert from 0xAARRGGBB
    static constexpr RGBColor fromARGB(uint32_t value)
    {
        return RGBColor{
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF),
            static_cast<uint8_t>((value >> 24) & 0xFF)
        };
    }

    // Convert from 0xRRGGBBAA
    static constexpr RGBColor fromRGBA(uint32_t value)
    {
        return RGBColor{
            static_cast<uint8_t>((value >> 24) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF)
        };
    }

    // Optional alias: treat `toRGBA()` as default `toInt()`
    constexpr uint32_t toInt() const
    {
        return toRGBA();
    }

    // Optional alias: treat `fromRGBA()` as default `fromInt()`
    static constexpr RGBColor fromInt(uint32_t value)
    {
        return fromRGBA(value);
    }


    // #RRGGBB or #RRGGBBAA
    [[nodiscard]]
    std::string toHexString(bool includeAlpha = false) const
    {
        char buf[10] = { '#' };
        const auto toHex = [](uint8_t value, char *out) {
            constexpr char hex[] = "0123456789ABCDEF";
            out[0] = hex[(value >> 4) & 0xF];
            out[1] = hex[value & 0xF];
        };

        toHex(r, buf + 1);
        toHex(g, buf + 3);
        toHex(b, buf + 5);

        if (includeAlpha) {
            toHex(a, buf + 7);
            buf[9] = '\0';
            return std::string(buf, 9);
        } else {
            buf[7] = '\0';
            return std::string(buf, 7);
        }
    }

    static RGBColor fromHexString(std::string_view hex)
    {
        if (hex.size() != 7 && hex.size() != 9)
            throw std::invalid_argument("Hex string must be #RRGGBB or #RRGGBBAA");

        if (hex[0] != '#')
            throw std::invalid_argument("Hex string must start with '#'");

        auto parseByte = [](std::string_view str) -> uint8_t {
            uint8_t value = 0;
            auto result = std::from_chars(str.data(), str.data() + str.size(), value, 16);
            if (result.ec != std::errc())
                throw std::invalid_argument("Invalid hex digit");

            return value;
        };

        uint8_t red   = parseByte(hex.substr(1, 2));
        uint8_t green = parseByte(hex.substr(3, 2));
        uint8_t blue  = parseByte(hex.substr(5, 2));
        uint8_t alpha = 255;
        if (hex.size() == 9)
            alpha = parseByte(hex.substr(7, 2));

        return RGBColor(red, green, blue, alpha);
    }
};

[[nodiscard]] inline std::optional<RGBColor> parseColorString(const std::string &input)
{
    try {
        return RGBColor::fromHexString(input);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

}
