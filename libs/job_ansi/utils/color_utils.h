#pragma once

#include <optional>
#include <array>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include "rgb_color.h"

namespace job::ansi::utils {

struct ParsedColor {
    bool isForeground;
    RGBColor color;
};

namespace detail {
inline constexpr RGBColor kBasicAnsiColorTable[8] = {
    { 0,   0,   0 },     // Black
    { 255, 0,   0 },     // Red
    { 0,   255, 0 },     // Green
    { 255, 255, 0 },     // Yellow
    { 0,   0,   255 },   // Blue
    { 255, 0,   255 },   // Magenta
    { 0,   255, 255 },   // Cyan
    { 255, 255, 255 }    // White
};

inline constexpr RGBColor kBrightAnsiColorTable[8] = {
    { 85,  85,  85 },    // Bright Black (Gray)
    { 255, 85,  85 },    // Bright Red
    { 85,  255, 85 },    // Bright Green
    { 255, 255, 85 },    // Bright Yellow
    { 85,  85,  255 },   // Bright Blue
    { 255, 85,  255 },   // Bright Magenta
    { 85,  255, 255 },   // Bright Cyan
    { 255, 255, 255 }    // Bright White
};
} // namespace detail


namespace colors {
inline RGBColor Black()
{
    return detail::kBasicAnsiColorTable[0];
}

inline RGBColor Red()
{
    return detail::kBasicAnsiColorTable[1];
}

inline RGBColor Green()
{
    return detail::kBasicAnsiColorTable[2];
}

inline RGBColor Yellow()
{
    return detail::kBasicAnsiColorTable[3];
}

inline RGBColor Blue()
{
    return detail::kBasicAnsiColorTable[4];
}

inline RGBColor Magenta()
{
    return detail::kBasicAnsiColorTable[5];
}

inline RGBColor Cyan()
{
    return detail::kBasicAnsiColorTable[6];
}

inline RGBColor White()
{
    return detail::kBasicAnsiColorTable[7];
}


inline RGBColor BrightBlack()
{
    return detail::kBrightAnsiColorTable[0];
}

inline RGBColor BrightRed()
{
    return detail::kBrightAnsiColorTable[1];
}

inline RGBColor BrightGreen()
{
    return detail::kBrightAnsiColorTable[2];
}

inline RGBColor BrightYellow()
{
    return detail::kBrightAnsiColorTable[3];
}

inline RGBColor BrightBlue()
{
    return detail::kBrightAnsiColorTable[4];
}

inline RGBColor BrightMagenta()
{
    return detail::kBrightAnsiColorTable[5];
}

inline RGBColor BrightCyan()
{
    return detail::kBrightAnsiColorTable[6];
}

inline RGBColor BrightWhite()
{
    return detail::kBrightAnsiColorTable[7];
}

} // namespace colors

inline std::optional<RGBColor> parseNamedOrHexColor(const std::string &name)
{
    static const std::unordered_map<std::string, RGBColor> kNamedColors =
        {
         // Basic
         { "black",   colors::Black() },
         { "red",     colors::Red() },
         { "green",   colors::Green() },
         { "yellow",  colors::Yellow() },
         { "blue",    colors::Blue() },
         { "magenta", colors::Magenta() },
         { "cyan",    colors::Cyan() },
         { "white",   colors::White() },

         // Bright
         { "bright-black",   colors::BrightBlack() },
         { "bright-red",     colors::BrightRed() },
         { "bright-green",   colors::BrightGreen() },
         { "bright-yellow",  colors::BrightYellow() },
         { "bright-blue",    colors::BrightBlue() },
         { "bright-magenta", colors::BrightMagenta() },
         { "bright-cyan",    colors::BrightCyan() },
         { "bright-white",   colors::BrightWhite() },
         };

    auto it = kNamedColors.find(name);
    if (it != kNamedColors.end())
        return it->second;

    return parseColorString(name); // fallback to hex
}

inline RGBColor lighter(RGBColor color, int percent)
{
    // Clamp percent to [0, 256]
    percent = std::clamp(percent, 0, 256);

    // Interpolate toward white (255,255,255)
    uint8_t r = color.red()   + ((255 - color.red())   * percent / 256);
    uint8_t g = color.green() + ((255 - color.green()) * percent / 256);
    uint8_t b = color.blue()  + ((255 - color.blue())  * percent / 256);
    return RGBColor{r, g, b, color.alpha()};
}

inline RGBColor darker(RGBColor color, int percent)
{
    // Clamp percent to [0, 256]
    percent = std::clamp(percent, 0, 256);

    // Interpolate toward black (0,0,0)
    uint8_t r = color.red()   * (256 - percent) / 256;
    uint8_t g = color.green() * (256 - percent) / 256;
    uint8_t b = color.blue()  * (256 - percent) / 256;
    return RGBColor{r, g, b, color.alpha()};
}

inline RGBColor basicAnsiColor(int index)
{
    return (index >= 0 && index < 8) ? detail::kBasicAnsiColorTable[index]
                                     : RGBColor{255, 255, 255};
}

inline RGBColor brightAnsiColor(int index)
{
    return (index >= 0 && index < 8) ? detail::kBrightAnsiColorTable[index]
                                     : RGBColor{255, 255, 255};
}

inline std::optional<ParsedColor> sgrCodeToColor(int code)
{
    if (code >= 30 && code <= 37)
        return ParsedColor{true, basicAnsiColor(code - 30)};
    else if (code >= 90 && code <= 97)
        return ParsedColor{true, brightAnsiColor(code - 90)};
    else if (code >= 40 && code <= 47)
        return ParsedColor{false, basicAnsiColor(code - 40)};
    else if (code >= 100 && code <= 107)
        return ParsedColor{false, brightAnsiColor(code - 100)};
    return std::nullopt;
}

inline RGBColor fromXterm256Palette(int colorCode)
{
    colorCode = std::clamp(colorCode, 0, 255);

    if (colorCode < 16) {
        // Standard + Bright colors
        return (colorCode < 8)
                   ? detail::kBasicAnsiColorTable[colorCode]
                   : detail::kBrightAnsiColorTable[colorCode - 8];
    } else if (colorCode < 232) {
        // 6x6x6 color cube
        int index = colorCode - 16;
        int r = index / 36;
        int g = (index / 6) % 6;
        int b = index % 6;

        return RGBColor {
            static_cast<uint8_t>(r ? r * 40 + 55 : 0),
            static_cast<uint8_t>(g ? g * 40 + 55 : 0),
            static_cast<uint8_t>(b ? b * 40 + 55 : 0)
        };
    } else {
        // Grayscale ramp
        uint8_t level = static_cast<uint8_t>(8 + (colorCode - 232) * 10);
        return RGBColor{level, level, level};
    }
}

inline const std::array<RGBColor, 256> &xterm256Palette()
{
    static std::array<RGBColor, 256> palette = [] {
        std::array<RGBColor, 256> arr;
        for (int i = 0; i < 256; ++i)
            arr[i] = fromXterm256Palette(i);
        return arr;
    }();
    return palette;
}

inline std::optional<int> rgbToSGR(const RGBColor &color, bool foreground)
{
    using namespace detail;

    for (int i = 0; i < 8; ++i) {
        if (color == kBasicAnsiColorTable[i])
            return (foreground ? 30 : 40) + i;
        if (color == kBrightAnsiColorTable[i])
            return (foreground ? 90 : 100) + i;
    }

    return std::nullopt;
}


inline std::string sgr256Color(int index, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") + ";5;" + std::to_string(index) + "m";
}


inline std::string sgrTrueColor(const RGBColor &color, bool foreground)
{
    return "\033[" + std::string(foreground ? "38" : "48") + ";2;" +
           std::to_string(color.red()) + ";" +
           std::to_string(color.green()) + ";" +
           std::to_string(color.blue()) + "m";
}

inline std::string sgrFromHex(const std::string &hex, bool foreground = true)
{
    auto opt = parseColorString(hex);
    if (!opt)
        return "";
    return sgrTrueColor(*opt, foreground);
}

inline int findNearestXterm256(const RGBColor &color)
{
    const auto &palette = xterm256Palette();
    int best = 0;
    int minDist = std::numeric_limits<int>::max();

    for (int i = 0; i < 256; ++i) {
        const auto &p = palette[i];
        int dr = int(p.red()) - int(color.red());
        int dg = int(p.green()) - int(color.green());
        int db = int(p.blue()) - int(color.blue());
        int dist = dr * dr + dg * dg + db * db;

        if (dist < minDist) {
            minDist = dist;
            best = i;
        }
    }

    return best;
}




}
