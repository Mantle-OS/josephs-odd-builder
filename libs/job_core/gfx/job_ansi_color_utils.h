#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "job_rgb.h"

namespace job::core {



class JobAnsiColorUtils {
public:

    JobAnsiColorUtils() = delete;
    ~JobAnsiColorUtils() = delete;

    struct ParsedAnsiColor
    {
        bool            isForeground{};
        JobRgb    color;
        [[nodiscard]] friend constexpr bool operator==(const ParsedAnsiColor &, const ParsedAnsiColor &) noexcept = default;
    };

    struct NamedAnsiColor
    {
        std::string_view    name;
        JobRgb        color;
        [[nodiscard]] friend constexpr bool operator==(const NamedAnsiColor &, const NamedAnsiColor &) noexcept = default;
    };

    // inline static constexpr std::array<JobRgb, 8>     kBasicAnsiColorTable;
    // inline static constexpr std::array<JobRgb, 8>     kBrightAnsiColorTable;
    // inline static constexpr std::array<NamedAnsiColor, 16>  kNamedColors;



    [[nodiscard]] static constexpr JobRgb fromXterm256Palette(int colorCode) noexcept
    {
        colorCode = std::clamp(colorCode, 0, 255);

        if (colorCode < 16) {
            if (colorCode < 8)
                return kBasicAnsiColorTable[static_cast<std::size_t>(colorCode)];

            return kBrightAnsiColorTable[static_cast<std::size_t>(colorCode - 8)];
        }

        if (colorCode < 232) {
            const int index = colorCode - 16;

            const int red = index / 36;
            const int green = (index / 6) % 6;
            const int blue = index % 6;

            return JobRgb{
                static_cast<std::uint8_t>(red != 0 ? red * 40 + 55 : 0),
                static_cast<std::uint8_t>(green != 0 ? green * 40 + 55 : 0),
                static_cast<std::uint8_t>(blue != 0 ? blue * 40 + 55 : 0)
            };
        }

        const auto level = static_cast<std::uint8_t>(8 + (colorCode - 232) * 10);
        return JobRgb{level, level, level};
    }


    [[nodiscard]] static constexpr JobRgb black() noexcept
    {
        return kBasicAnsiColorTable[0];
    }

    [[nodiscard]] static constexpr JobRgb red() noexcept
    {
        return kBasicAnsiColorTable[1];
    }

    [[nodiscard]] static constexpr JobRgb green() noexcept
    {
        return kBasicAnsiColorTable[2];
    }

    [[nodiscard]] static constexpr JobRgb yellow() noexcept
    {
        return kBasicAnsiColorTable[3];
    }

    [[nodiscard]] static constexpr JobRgb blue() noexcept
    {
        return kBasicAnsiColorTable[4];
    }

    [[nodiscard]] static constexpr JobRgb magenta() noexcept
    {
        return kBasicAnsiColorTable[5];
    }

    [[nodiscard]] static constexpr JobRgb cyan() noexcept
    {
        return kBasicAnsiColorTable[6];
    }

    [[nodiscard]] static constexpr JobRgb white() noexcept
    {
        return kBasicAnsiColorTable[7];
    }

    [[nodiscard]] static constexpr JobRgb brightBlack() noexcept
    {
        return kBrightAnsiColorTable[0];
    }

    [[nodiscard]] static constexpr JobRgb brightRed() noexcept
    {
        return kBrightAnsiColorTable[1];
    }

    [[nodiscard]] static constexpr JobRgb brightGreen() noexcept
    {
        return kBrightAnsiColorTable[2];
    }

    [[nodiscard]] static constexpr JobRgb brightYellow() noexcept
    {
        return kBrightAnsiColorTable[3];
    }

    [[nodiscard]] static constexpr JobRgb brightBlue() noexcept
    {
        return kBrightAnsiColorTable[4];
    }

    [[nodiscard]] static constexpr JobRgb brightMagenta() noexcept
    {
        return kBrightAnsiColorTable[5];
    }

    [[nodiscard]] static constexpr JobRgb brightCyan() noexcept
    {
        return kBrightAnsiColorTable[6];
    }

    [[nodiscard]] static constexpr JobRgb brightWhite() noexcept
    {
        return kBrightAnsiColorTable[7];
    }


    [[nodiscard]] static constexpr std::optional<JobRgb> parseNamedOrHexColor(std::string_view name) noexcept
    {
        for (const auto &entry : kNamedColors) {
            if (entry.name == name)
                return entry.color;
        }
        // This is fine for now.
        return JobRgb::parseColorString(name);
    }

    // // Should move over to color utils or even JobRgb color we will see
    // [[nodiscard]] inline constexpr JobRgb lighter(JobRgb color, int percent) noexcept

    // // Should move over to color utils or even JobRgb color we will see
    // [[nodiscard]] inline constexpr JobRgb darker(JobRgb color, int percent) noexcept


    [[nodiscard]] static constexpr JobRgb basicAnsiColor(int index) noexcept
    {
        if (index < 0 || index >= static_cast<int>(kBasicAnsiColorTable.size()))
            return JobRgb{255, 255, 255, 255};

        return kBasicAnsiColorTable[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] static constexpr JobRgb brightAnsiColor(int index) noexcept
    {
        if (index < 0 || index >= static_cast<int>(kBrightAnsiColorTable.size()))
            return JobRgb{255, 255, 255, 255};

        return kBrightAnsiColorTable[static_cast<std::size_t>(index)];
    }

    [[nodiscard]] static constexpr std::optional<ParsedAnsiColor> sgrCodeToColor(int code) noexcept
    {
        if (code >= 30 && code <= 37)
            return ParsedAnsiColor{true, basicAnsiColor(code - 30)};

        if (code >= 90 && code <= 97)
            return ParsedAnsiColor{true, brightAnsiColor(code - 90)};

        if (code >= 40 && code <= 47)
            return ParsedAnsiColor{false, basicAnsiColor(code - 40)};

        if (code >= 100 && code <= 107)
            return ParsedAnsiColor{false, brightAnsiColor(code - 100)};

        return std::nullopt;
    }

    [[nodiscard]] static constexpr const std::array<JobRgb, 256> &xterm256Palette() noexcept
    {
        return kXterm256Palette;
    }

    [[nodiscard]] static constexpr std::optional<int> rgbToSGR(const JobRgb &color, bool foreground) noexcept
    {
        for (std::size_t i = 0; i < kBasicAnsiColorTable.size(); ++i) {
            if (color == kBasicAnsiColorTable[i])
                return (foreground ? 30 : 40) + static_cast<int>(i);

            if (color == kBrightAnsiColorTable[i])
                return (foreground ? 90 : 100) + static_cast<int>(i);
        }

        return std::nullopt;
    }

    [[nodiscard]] static std::string sgr256Color(int index, bool foreground)
    {
        return "\033[" + std::string(foreground ? "38" : "48") +
               ";5;" + std::to_string(index) + "m";
    }

    [[nodiscard]] static std::string sgrTrueColor(const JobRgb &color, bool foreground)
    {
        return "\033[" + std::string(foreground ? "38" : "48") +
               ";2;" +
               std::to_string(color.r) + ";" +
               std::to_string(color.g) + ";" +
               std::to_string(color.b) + "m";
    }

    [[nodiscard]] static std::string sgrFromHex(std::string_view hex, bool foreground = true)
    {
        const auto color = JobRgb::parseColorString(hex);
        if (!color)
            return {};

        return sgrTrueColor(*color, foreground);
    }

    [[nodiscard]] static constexpr int findNearestXterm256(const JobRgb &color) noexcept
    {
        const auto &palette = xterm256Palette();

        int best = 0;
        int minimumDistance = std::numeric_limits<int>::max();

        for (std::size_t i = 0; i < palette.size(); ++i) {
            const auto &candidate = palette[i];
            const int redDelta = static_cast<int>(candidate.r) - static_cast<int>(color.r);
            const int greenDelta = static_cast<int>(candidate.g) - static_cast<int>(color.g);
            const int blueDelta = static_cast<int>(candidate.b) - static_cast<int>(color.b);
            const int distance = redDelta * redDelta + greenDelta * greenDelta + blueDelta * blueDelta;
            if (distance < minimumDistance) {
                minimumDistance = distance;
                best = static_cast<int>(i);
            }
        }

        return best;
    }

private:
    [[nodiscard]] static constexpr std::array<JobRgb, 256> makeXterm256Palette() noexcept
    {
        std::array<JobRgb, 256> palette{};
        for (std::size_t i = 0; i < palette.size(); ++i)
            palette[i] = fromXterm256Palette(static_cast<int>(i));

        return palette;
    }

    static constexpr std::array<JobRgb, 8> kBasicAnsiColorTable{{
        {0,   0,   0, 255},       // Black
        {255, 0,   0, 255},       // Red
        {0,   255, 0, 255},       // Green
        {255, 255, 0, 255},       // Yellow
        {0,   0,   255, 255},     // Blue
        {255, 0,   255, 255},     // Magenta
        {0,   255, 255, 255},     // Cyan
        {255, 255, 255, 255}      // White
    }};

    static constexpr std::array<JobRgb, 8> kBrightAnsiColorTable{{
        {85,  85,  85 , 255},      // Bright Black
        {255, 85,  85 , 255},      // Bright Red
        {85,  255, 85 , 255},      // Bright Green
        {255, 255, 85 , 255},      // Bright Yellow
        {85,  85,  255, 255},     // Bright Blue
        {255, 85,  255, 255},     // Bright Magenta
        {85,  255, 255, 255},     // Bright Cyan
        {255, 255, 255, 255}      // Bright White
    }};

    static constexpr std::array<NamedAnsiColor, 16> kNamedColors{{
        {"black",          {0,   0,   0, 255}},
        {"red",            {255, 0,   0, 255}},
        {"green",          {0,   255, 0, 255}},
        {"yellow",         {255, 255, 0, 255}},
        {"blue",           {0,   0,   255, 255}},
        {"magenta",        {255, 0,   255, 255}},
        {"cyan",           {0,   255, 255, 255}},
        {"white",          {255, 255, 255, 255}},
        {"bright-black",   {85,  85,  85 , 255}},
        {"bright-red",     {255, 85,  85 , 255}},
        {"bright-green",   {85,  255, 85 , 255}},
        {"bright-yellow",  {255, 255, 85 , 255}},
        {"bright-blue",    {85,  85,  255, 255}},
        {"bright-magenta", {255, 85,  255, 255}},
        {"bright-cyan",    {85,  255, 255, 255}},
        {"bright-white",   {255, 255, 255, 255}}
    }};
};


}