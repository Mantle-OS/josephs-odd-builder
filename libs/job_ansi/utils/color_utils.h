#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "rgb_color.h"

namespace job::ansi::utils {

struct ParsedColor
{
    bool isForeground;
    RGBColor color;
};

namespace detail {

inline constexpr std::array<RGBColor, 8> kBasicAnsiColorTable{{
    { 0,   0,   0 },     // Black
    { 255, 0,   0 },     // Red
    { 0,   255, 0 },     // Green
    { 255, 255, 0 },     // Yellow
    { 0,   0,   255 },   // Blue
    { 255, 0,   255 },   // Magenta
    { 0,   255, 255 },   // Cyan
    { 255, 255, 255 }    // White
}};

inline constexpr std::array<RGBColor, 8> kBrightAnsiColorTable{{
    { 85,  85,  85 },    // Bright Black (Gray)
    { 255, 85,  85 },    // Bright Red
    { 85,  255, 85 },    // Bright Green
    { 255, 255, 85 },    // Bright Yellow
    { 85,  85,  255 },   // Bright Blue
    { 255, 85,  255 },   // Bright Magenta
    { 85,  255, 255 },   // Bright Cyan
    { 255, 255, 255 }    // Bright White
}};

struct NamedColor
{
    std::string_view name;
    RGBColor color;
};

inline constexpr std::array<NamedColor, 16> kNamedColors{{
    { "black",          kBasicAnsiColorTable[0] },
    { "red",            kBasicAnsiColorTable[1] },
    { "green",          kBasicAnsiColorTable[2] },
    { "yellow",         kBasicAnsiColorTable[3] },
    { "blue",           kBasicAnsiColorTable[4] },
    { "magenta",        kBasicAnsiColorTable[5] },
    { "cyan",           kBasicAnsiColorTable[6] },
    { "white",          kBasicAnsiColorTable[7] },

    { "bright-black",   kBrightAnsiColorTable[0] },
    { "bright-red",     kBrightAnsiColorTable[1] },
    { "bright-green",   kBrightAnsiColorTable[2] },
    { "bright-yellow",  kBrightAnsiColorTable[3] },
    { "bright-blue",    kBrightAnsiColorTable[4] },
    { "bright-magenta", kBrightAnsiColorTable[5] },
    { "bright-cyan",    kBrightAnsiColorTable[6] },
    { "bright-white",   kBrightAnsiColorTable[7] }
}};

} // namespace detail

namespace colors {

[[nodiscard]] constexpr RGBColor Black() noexcept
{
    return detail::kBasicAnsiColorTable[0];
}

[[nodiscard]] constexpr RGBColor Red() noexcept
{
    return detail::kBasicAnsiColorTable[1];
}

[[nodiscard]] constexpr RGBColor Green() noexcept
{
    return detail::kBasicAnsiColorTable[2];
}

[[nodiscard]] constexpr RGBColor Yellow() noexcept
{
    return detail::kBasicAnsiColorTable[3];
}

[[nodiscard]] constexpr RGBColor Blue() noexcept
{
    return detail::kBasicAnsiColorTable[4];
}

[[nodiscard]] constexpr RGBColor Magenta() noexcept
{
    return detail::kBasicAnsiColorTable[5];
}

[[nodiscard]] constexpr RGBColor Cyan() noexcept
{
    return detail::kBasicAnsiColorTable[6];
}

[[nodiscard]] constexpr RGBColor White() noexcept
{
    return detail::kBasicAnsiColorTable[7];
}

[[nodiscard]] constexpr RGBColor BrightBlack() noexcept
{
    return detail::kBrightAnsiColorTable[0];
}

[[nodiscard]] constexpr RGBColor BrightRed() noexcept
{
    return detail::kBrightAnsiColorTable[1];
}

[[nodiscard]] constexpr RGBColor BrightGreen() noexcept
{
    return detail::kBrightAnsiColorTable[2];
}

[[nodiscard]] constexpr RGBColor BrightYellow() noexcept
{
    return detail::kBrightAnsiColorTable[3];
}

[[nodiscard]] constexpr RGBColor BrightBlue() noexcept
{
    return detail::kBrightAnsiColorTable[4];
}

[[nodiscard]] constexpr RGBColor BrightMagenta() noexcept
{
    return detail::kBrightAnsiColorTable[5];
}

[[nodiscard]] constexpr RGBColor BrightCyan() noexcept
{
    return detail::kBrightAnsiColorTable[6];
}

[[nodiscard]] constexpr RGBColor BrightWhite() noexcept
{
    return detail::kBrightAnsiColorTable[7];
}

} // namespace colors

[[nodiscard]] inline std::optional<RGBColor> parseNamedOrHexColor(const std::string &name)
{
    for (const auto &[namedColor, color] : detail::kNamedColors) {
        if (name == namedColor)
            return color;
    }

    return parseColorString(name);
}

[[nodiscard]] constexpr RGBColor lighter(RGBColor color, int percent) noexcept
{
    percent = std::clamp(percent, 0, 256);

    const auto red = static_cast<std::uint8_t>(
        color.red() + ((255 - color.red()) * percent / 256));

    const auto green = static_cast<std::uint8_t>(
        color.green() + ((255 - color.green()) * percent / 256));

    const auto blue = static_cast<std::uint8_t>(
        color.blue() + ((255 - color.blue()) * percent / 256));

    return RGBColor{red, green, blue, color.alpha()};
}

[[nodiscard]] constexpr RGBColor darker(RGBColor color, int percent) noexcept
{
    percent = std::clamp(percent, 0, 256);

    const auto red = static_cast<std::uint8_t>(
        color.red() * (256 - percent) / 256);

    const auto green = static_cast<std::uint8_t>(
        color.green() * (256 - percent) / 256);

    const auto blue = static_cast<std::uint8_t>(
        color.blue() * (256 - percent) / 256);

    return RGBColor{red, green, blue, color.alpha()};
}

[[nodiscard]] constexpr RGBColor basicAnsiColor(int index) noexcept
{
    if (index < 0 || index >= static_cast<int>(detail::kBasicAnsiColorTable.size()))
        return RGBColor{255, 255, 255};

    return detail::kBasicAnsiColorTable[static_cast<std::size_t>(index)];
}

[[nodiscard]] constexpr RGBColor brightAnsiColor(int index) noexcept
{
    if (index < 0 || index >= static_cast<int>(detail::kBrightAnsiColorTable.size()))
        return RGBColor{255, 255, 255};

    return detail::kBrightAnsiColorTable[static_cast<std::size_t>(index)];
}

[[nodiscard]] constexpr std::optional<ParsedColor> sgrCodeToColor(int code) noexcept
{
    if (code >= 30 && code <= 37)
        return ParsedColor{true, basicAnsiColor(code - 30)};

    if (code >= 90 && code <= 97)
        return ParsedColor{true, brightAnsiColor(code - 90)};

    if (code >= 40 && code <= 47)
        return ParsedColor{false, basicAnsiColor(code - 40)};

    if (code >= 100 && code <= 107)
        return ParsedColor{false, brightAnsiColor(code - 100)};

    return std::nullopt;
}

[[nodiscard]] constexpr RGBColor fromXterm256Palette(int colorCode) noexcept
{
    colorCode = std::clamp(colorCode, 0, 255);

    if (colorCode < 16) {
        if (colorCode < 8)
            return detail::kBasicAnsiColorTable[static_cast<std::size_t>(colorCode)];

        return detail::kBrightAnsiColorTable[static_cast<std::size_t>(colorCode - 8)];
    }

    if (colorCode < 232) {
        const int index = colorCode - 16;
        const int red = index / 36;
        const int green = (index / 6) % 6;
        const int blue = index % 6;

        return RGBColor{
            static_cast<std::uint8_t>(red ? red * 40 + 55 : 0),
            static_cast<std::uint8_t>(green ? green * 40 + 55 : 0),
            static_cast<std::uint8_t>(blue ? blue * 40 + 55 : 0)
        };
    }

    const auto level = static_cast<std::uint8_t>(8 + (colorCode - 232) * 10);
    return RGBColor{level, level, level};
}

inline constexpr std::array<RGBColor, 256> kXterm256Palette = [] {
    std::array<RGBColor, 256> palette{};

    for (std::size_t index = 0; index < palette.size(); ++index)
        palette[index] = fromXterm256Palette(static_cast<int>(index));

    return palette;
}();

[[nodiscard]] constexpr const std::array<RGBColor, 256> &xterm256Palette() noexcept
{
    return kXterm256Palette;
}

[[nodiscard]] constexpr std::optional<int> rgbToSGR(const RGBColor &color, bool foreground) noexcept
{
    for (std::size_t index = 0; index < detail::kBasicAnsiColorTable.size(); ++index) {
        if (color == detail::kBasicAnsiColorTable[index])
            return (foreground ? 30 : 40) + static_cast<int>(index);

        if (color == detail::kBrightAnsiColorTable[index])
            return (foreground ? 90 : 100) + static_cast<int>(index);
    }

    return std::nullopt;
}

[[nodiscard]] inline std::string sgr256Color(int index, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") + ";5;" + std::to_string(index) + "m";
}

[[nodiscard]] inline std::string sgrTrueColor(const RGBColor &color, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") + ";2;" +
           std::to_string(color.red()) + ";" +
           std::to_string(color.green()) + ";" +
           std::to_string(color.blue()) + "m";
}

[[nodiscard]] inline std::string sgrFromHex(const std::string &hex, bool foreground = true)
{
    const auto color = parseColorString(hex);

    if (!color)
        return {};

    return sgrTrueColor(*color, foreground);
}

[[nodiscard]] constexpr int findNearestXterm256(const RGBColor &color) noexcept
{
    int best = 0;
    int minDistance = std::numeric_limits<int>::max();

    for (std::size_t index = 0; index < kXterm256Palette.size(); ++index) {
        const RGBColor &paletteColor = kXterm256Palette[index];
        const int red   = static_cast<int>(paletteColor.red()) - static_cast<int>(color.red());
        const int green = static_cast<int>(paletteColor.green()) - static_cast<int>(color.green());
        const int blue  = static_cast<int>(paletteColor.blue()) - static_cast<int>(color.blue());

        const int distance = red * red + green * green + blue * blue;
        if (distance < minDistance) {
            minDistance = distance;
            best = static_cast<int>(index);
        }
    }

    return best;
}

} // namespace job::ansi::utils