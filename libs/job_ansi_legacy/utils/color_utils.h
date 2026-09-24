#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "rgb_color.h"

namespace job::ansi::utils {

struct ParsedColor
{
    bool isForeground{};
    RGBColor color;
    [[nodiscard]] friend constexpr bool operator==(const ParsedColor &, const ParsedColor &) noexcept = default;
};

namespace detail {

inline constexpr std::array<RGBColor, 8> kBasicAnsiColorTable{{
    {0,   0,   0},       // Black
    {255, 0,   0},       // Red
    {0,   255, 0},       // Green
    {255, 255, 0},       // Yellow
    {0,   0,   255},     // Blue
    {255, 0,   255},     // Magenta
    {0,   255, 255},     // Cyan
    {255, 255, 255}      // White
}};

inline constexpr std::array<RGBColor, 8> kBrightAnsiColorTable{{
    {85,  85,  85},      // Bright Black
    {255, 85,  85},      // Bright Red
    {85,  255, 85},      // Bright Green
    {255, 255, 85},      // Bright Yellow
    {85,  85,  255},     // Bright Blue
    {255, 85,  255},     // Bright Magenta
    {85,  255, 255},     // Bright Cyan
    {255, 255, 255}      // Bright White
}};

} // namespace detail

namespace colors {

[[nodiscard]] inline constexpr RGBColor Black() noexcept
{
    return detail::kBasicAnsiColorTable[0];
}

[[nodiscard]] inline constexpr RGBColor Red() noexcept
{
    return detail::kBasicAnsiColorTable[1];
}

[[nodiscard]] inline constexpr RGBColor Green() noexcept
{
    return detail::kBasicAnsiColorTable[2];
}

[[nodiscard]] inline constexpr RGBColor Yellow() noexcept
{
    return detail::kBasicAnsiColorTable[3];
}

[[nodiscard]] inline constexpr RGBColor Blue() noexcept
{
    return detail::kBasicAnsiColorTable[4];
}

[[nodiscard]] inline constexpr RGBColor Magenta() noexcept
{
    return detail::kBasicAnsiColorTable[5];
}

[[nodiscard]] inline constexpr RGBColor Cyan() noexcept
{
    return detail::kBasicAnsiColorTable[6];
}

[[nodiscard]] inline constexpr RGBColor White() noexcept
{
    return detail::kBasicAnsiColorTable[7];
}

[[nodiscard]] inline constexpr RGBColor BrightBlack() noexcept
{
    return detail::kBrightAnsiColorTable[0];
}

[[nodiscard]] inline constexpr RGBColor BrightRed() noexcept
{
    return detail::kBrightAnsiColorTable[1];
}

[[nodiscard]] inline constexpr RGBColor BrightGreen() noexcept
{
    return detail::kBrightAnsiColorTable[2];
}

[[nodiscard]] inline constexpr RGBColor BrightYellow() noexcept
{
    return detail::kBrightAnsiColorTable[3];
}

[[nodiscard]] inline constexpr RGBColor BrightBlue() noexcept
{
    return detail::kBrightAnsiColorTable[4];
}

[[nodiscard]] inline constexpr RGBColor BrightMagenta() noexcept
{
    return detail::kBrightAnsiColorTable[5];
}

[[nodiscard]] inline constexpr RGBColor BrightCyan() noexcept
{
    return detail::kBrightAnsiColorTable[6];
}

[[nodiscard]] inline constexpr RGBColor BrightWhite() noexcept
{
    return detail::kBrightAnsiColorTable[7];
}

} // namespace colors

namespace detail {

struct NamedColor
{
    std::string_view    name;
    RGBColor            color;
    [[nodiscard]] friend constexpr bool operator==(const NamedColor &, const NamedColor &) noexcept = default;
};

inline constexpr std::array<NamedColor, 16> kNamedColors{{
    {"black",          colors::Black()},
    {"red",            colors::Red()},
    {"green",          colors::Green()},
    {"yellow",         colors::Yellow()},
    {"blue",           colors::Blue()},
    {"magenta",        colors::Magenta()},
    {"cyan",           colors::Cyan()},
    {"white",          colors::White()},
    {"bright-black",   colors::BrightBlack()},
    {"bright-red",     colors::BrightRed()},
    {"bright-green",   colors::BrightGreen()},
    {"bright-yellow",  colors::BrightYellow()},
    {"bright-blue",    colors::BrightBlue()},
    {"bright-magenta", colors::BrightMagenta()},
    {"bright-cyan",    colors::BrightCyan()},
    {"bright-white",   colors::BrightWhite()}
}};

} // namespace detail

[[nodiscard]] inline constexpr std::optional<RGBColor> parseNamedOrHexColor(std::string_view name) noexcept
{
    for (const auto &entry : detail::kNamedColors) {
        if (entry.name == name)
            return entry.color;
    }

    return parseColorString(name);
}

[[nodiscard]] inline constexpr RGBColor lighter(RGBColor color, int percent) noexcept
{
    percent = std::clamp(percent, 0, 256);

    const int red = static_cast<int>(color.red()) +
                    ((255 - static_cast<int>(color.red())) * percent / 256);

    const int green = static_cast<int>(color.green()) +
                      ((255 - static_cast<int>(color.green())) * percent / 256);

    const int blue = static_cast<int>(color.blue()) +
                     ((255 - static_cast<int>(color.blue())) * percent / 256);

    return RGBColor{
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        color.alpha()
    };
}

[[nodiscard]] inline constexpr RGBColor darker(RGBColor color, int percent) noexcept
{
    percent = std::clamp(percent, 0, 256);

    const int scale = 256 - percent;

    const int red = static_cast<int>(color.red()) * scale / 256;
    const int green = static_cast<int>(color.green()) * scale / 256;
    const int blue = static_cast<int>(color.blue()) * scale / 256;

    return RGBColor{
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        color.alpha()
    };
}

[[nodiscard]] inline constexpr RGBColor basicAnsiColor(int index) noexcept
{
    if (index < 0 || index >= static_cast<int>(detail::kBasicAnsiColorTable.size()))
        return RGBColor{255, 255, 255};

    return detail::kBasicAnsiColorTable[static_cast<std::size_t>(index)];
}

[[nodiscard]] inline constexpr RGBColor brightAnsiColor(int index) noexcept
{
    if (index < 0 || index >= static_cast<int>(detail::kBrightAnsiColorTable.size()))
        return RGBColor{255, 255, 255};

    return detail::kBrightAnsiColorTable[static_cast<std::size_t>(index)];
}

[[nodiscard]] inline constexpr std::optional<ParsedColor> sgrCodeToColor(int code) noexcept
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

[[nodiscard]] inline constexpr RGBColor fromXterm256Palette(int colorCode) noexcept
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
            static_cast<std::uint8_t>(red != 0 ? red * 40 + 55 : 0),
            static_cast<std::uint8_t>(green != 0 ? green * 40 + 55 : 0),
            static_cast<std::uint8_t>(blue != 0 ? blue * 40 + 55 : 0)
        };
    }

    const auto level = static_cast<std::uint8_t>(8 + (colorCode - 232) * 10);
    return RGBColor{level, level, level};
}

namespace detail {

[[nodiscard]] inline constexpr std::array<RGBColor, 256> makeXterm256Palette() noexcept
{
    std::array<RGBColor, 256> palette{};

    for (std::size_t i = 0; i < palette.size(); ++i)
        palette[i] = fromXterm256Palette(static_cast<int>(i));

    return palette;
}

inline constexpr auto kXterm256Palette = makeXterm256Palette();

} // namespace detail

[[nodiscard]] inline constexpr const std::array<RGBColor, 256> &xterm256Palette() noexcept
{
    return detail::kXterm256Palette;
}

[[nodiscard]] inline constexpr std::optional<int> rgbToSGR(const RGBColor &color, bool foreground) noexcept
{
    for (std::size_t i = 0; i < detail::kBasicAnsiColorTable.size(); ++i) {
        if (color == detail::kBasicAnsiColorTable[i])
            return (foreground ? 30 : 40) + static_cast<int>(i);

        if (color == detail::kBrightAnsiColorTable[i])
            return (foreground ? 90 : 100) + static_cast<int>(i);
    }

    return std::nullopt;
}

[[nodiscard]] inline std::string sgr256Color(int index, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") +
           ";5;" + std::to_string(index) + "m";
}

[[nodiscard]] inline std::string sgrTrueColor(const RGBColor &color, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") +
           ";2;" +
           std::to_string(color.red()) + ";" +
           std::to_string(color.green()) + ";" +
           std::to_string(color.blue()) + "m";
}

[[nodiscard]] inline std::string sgrFromHex(std::string_view hex, bool foreground = true)
{
    const auto color = parseColorString(hex);

    if (!color)
        return {};

    return sgrTrueColor(*color, foreground);
}

[[nodiscard]] inline constexpr int findNearestXterm256(const RGBColor &color) noexcept
{
    const auto &palette = xterm256Palette();

    int best = 0;
    int minimumDistance = std::numeric_limits<int>::max();

    for (std::size_t i = 0; i < palette.size(); ++i) {
        const auto &candidate = palette[i];
        const int redDelta = static_cast<int>(candidate.red()) - static_cast<int>(color.red());
        const int greenDelta = static_cast<int>(candidate.green()) - static_cast<int>(color.green());
        const int blueDelta = static_cast<int>(candidate.blue()) - static_cast<int>(color.blue());
        const int distance = redDelta * redDelta + greenDelta * greenDelta + blueDelta * blueDelta;
        if (distance < minimumDistance) {
            minimumDistance = distance;
            best = static_cast<int>(i);
        }
    }

    return best;
}

} // namespace job::ansi::utils