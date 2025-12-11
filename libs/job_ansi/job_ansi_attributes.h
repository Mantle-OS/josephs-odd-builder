#pragma once

#include <optional>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "utils/rgb_color.h"
#include "utils/job_ansi_enums.h"

namespace job::ansi {

using namespace job::ansi::utils::enums;
using job::ansi::utils::RGBColor;

class Attributes {
public:

    using Ptr = std::shared_ptr<Attributes>;

    union {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        struct {
            uint16_t bold          : 1;
            uint16_t faint         : 1;
            uint16_t italic        : 1;
            uint16_t strikethrough : 1;
            uint16_t inverse       : 1;
            uint16_t conceal       : 1;
            uint16_t framed        : 1;
            uint16_t encircled     : 1;
            uint16_t overline      : 1;
            uint16_t reserved      : 5;
            uint16_t blink         : 2;  // 0=none, 1=slow, 2=rapid
        } ;
        uint16_t flags = 0;
    };

    UnderlineStyle underline = UnderlineStyle::None;

    static std::string_view underlineToString(UnderlineStyle style)
    {
        switch (style) {
            case UnderlineStyle::None:
            return "none";
            case UnderlineStyle::Single:
                return "single";
            case UnderlineStyle::Double:
                return "double";
            case UnderlineStyle::Curly:
                return "curly";
            case UnderlineStyle::Dotted:
                return "dotted";
            case UnderlineStyle::Dashed:
                return "dashed";
        }
        return "unknown";
    }

    std::optional<RGBColor> foreground;
    std::optional<RGBColor> background;

    /**
     * @brief Construct default attributes (reset SGR)
     */
    Attributes()
    {
        flags = 0;
        underline = UnderlineStyle::None;
        foreground.reset();
        background.reset();
    }

    void setForeground(const RGBColor &color)
    {
        foreground = color;
    }

    void setBackground(const RGBColor &color)
    {
        background = color;
    }

    void resetColors()
    {
        foreground.reset();
        background.reset();
    }

    void setUnderline(UnderlineStyle style)
    {
        underline = style;
    }

    void setBlink(uint8_t mode)
    {
        blink = mode & 0x3;
    }

    std::string toString() const
    {
        std::string str = "flags=" + std::to_string(flags);
        str += " underline=" + std::to_string(static_cast<int>(underline));
        if (foreground)
            str += " fg=" + foreground->toHexString();
        if (background)
            str += " bg=" + background->toHexString();
        return str;
    }

    void reset()
    {
        bold = faint = italic = strikethrough = inverse =
            conceal = framed = encircled = overline = false;
        blink = 0;
        flags = 0;  // Redundant, but safe if layout changes
        underline = UnderlineStyle::None;
        foreground.reset();
        background.reset();
    }
    bool operator==(const Attributes &other) const
    {
        return flags == other.flags &&
               underline == other.underline &&
               foreground == other.foreground &&
               background == other.background;
    }

    bool operator!=(const Attributes &other) const
    {
        return !(*this == other);
    }

    struct Hash
    {
        std::size_t operator()(const Attributes &attr) const {
            std::size_t h1 = std::hash<uint16_t>{}(attr.flags);
            std::size_t h2 = std::hash<int>{}(static_cast<int>(attr.underline));
            std::size_t h3 = attr.foreground ?
                                 std::hash<uint32_t>{}(attr.foreground->toInt()) :
                                 0;
            std::size_t h4 = attr.background ?
                                 std::hash<uint32_t>{}(attr.background->toInt()) :
                                 0;
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    static Attributes::Ptr intern(const Attributes &value)
    {
        static std::unordered_map<Attributes, std::weak_ptr<Attributes>, Hash> cache;
        static std::mutex mutex;

        std::lock_guard<std::mutex> lock(mutex);

        auto it = cache.find(value);
        if (it != cache.end()) {
            if (auto shared = it->second.lock())
                return shared;
            cache.erase(it);
        }

        auto shared = std::make_shared<Attributes>(value);
        cache.emplace(value, shared);
        return shared;
    }
};

}
