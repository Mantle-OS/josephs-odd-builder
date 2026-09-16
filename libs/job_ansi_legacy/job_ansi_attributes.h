#pragma once

#include <optional>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "utils/rgb_color.h"
#include "utils/job_ansi_enums.h"


// Backlog Item: Add a static assert to ensure the sizes match: static_assert(sizeof(Attributes) == sizeof(uint16_t) + ...) or similar, just to catch if a compiler update adds padding.
// Backlog Item: Implement a "Garbage Collection" sweep (e.g., scan and remove expired entries every N inserts) or a custom deleter strategy
// Backlog Item: Consider std::shared_mutex (Read/Write lock) or sharding the cache if performance profiling shows contention.

namespace job::ansi {

using namespace job::ansi::utils::enums;
using job::ansi::utils::RGBColor;

class Attributes {
public:
    using Ptr = std::shared_ptr<Attributes>;

    Attributes();
    Attributes(const Attributes&) = default;
    Attributes(Attributes&&) noexcept = default;

    [[nodiscard]] bool operator==(const Attributes &other) const noexcept;
    [[nodiscard]] bool operator!=(const Attributes &other) const noexcept;

    Attributes& operator=(const Attributes&) = default;
    Attributes& operator=(Attributes&&) noexcept = default;

    ~Attributes() = default;

    [[nodiscard]] const RGBColor *getForeground() const noexcept;
    void setForeground(const RGBColor &color) noexcept;
    void resetForeground() noexcept;

    [[nodiscard]] const RGBColor *getBackground() const noexcept;
    void setBackground(const RGBColor &color) noexcept;    
    void resetBackground() noexcept;

    void resetColors() noexcept;
    void setBlink(uint8_t mode) noexcept;

    UnderlineStyle getUnderline() const noexcept;
    void setUnderline(UnderlineStyle style) noexcept;
    [[nodiscard]] static std::string_view underlineToString(UnderlineStyle style) noexcept;

    [[nodiscard]] std::string toString() const;
    void reset();

    struct Hash {
        std::size_t operator()(const Attributes &attr) const {
            // Hash the flags (includes bools, underline style, and hasColor bits)
            std::size_t h = std::hash<uint16_t>{}(attr.flags);

            // Only hash colors if the validity bit is set
            if (attr.hasFg) {
                h ^= std::hash<uint32_t>{}(attr.m_fg.toInt()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            if (attr.hasBg) {
                h ^= std::hash<uint32_t>{}(attr.m_bg.toInt()) + 0x9e3779b9 + (h << 6) + (h >> 2);
            }
            return h;
        }
    };

    static Attributes::Ptr intern(const Attributes &value);

public: // Direct access allowed for bitfields (standard C++ practice for POD-like types)
    union {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        struct {
            // Style Flags (9 bits)
            uint16_t bold          : 1;
            uint16_t faint         : 1;
            uint16_t italic        : 1;
            uint16_t strikethrough : 1;
            uint16_t inverse       : 1;
            uint16_t conceal       : 1;
            uint16_t framed        : 1;
            uint16_t encircled     : 1;
            uint16_t overline      : 1;

            // Blink (2 bits)
            uint16_t blink         : 2;  // 0=none, 1=slow, 2=rapid

            // Packed Underline Style (3 bits, fits 0-7)
            uint16_t underlineVal  : 3;

            // Packed "Has Color" flags (2 bits) -> Replaces std::optional overhead
            uint16_t hasFg         : 1;
            uint16_t hasBg         : 1;
        };
        // Total: 16 bits = 2 bytes
        uint16_t flags = 0;
#pragma GCC diagnostic pop
    };

private:
    // Raw storage for colors (8 bytes). Validity determined by flags.
    // Memory Layout: 4 (FG) + 4 (BG) + 2 (Flags) + 2 (Padding) = 12 Bytes total
    RGBColor m_fg;
    RGBColor m_bg;


};
static_assert(sizeof(Attributes) <= 12, "Attributes class has bloated beyond 12 bytes!");
}
