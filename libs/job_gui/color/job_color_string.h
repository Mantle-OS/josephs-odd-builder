#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include <job_color_type.h>
#include <job_string_utils.h>

#include "job_color_keywords.h"

namespace job::gui {

class JobColorString
{
public:
    using ColorPair = std::pair<core::JobColorType, std::uint64_t>;

    JobColorString() = delete;
    ~JobColorString() = delete;

    JobColorString(const JobColorString &) = delete;
    JobColorString &operator=(const JobColorString &) = delete;

    JobColorString(JobColorString &&) noexcept = delete;
    JobColorString &operator=(JobColorString &&) noexcept = delete;

    // Longest string is "lightgoldenrodyellow".
    // Shortest string is "red", or others of length 3.
    [[nodiscard]] static ColorPair parseColor(std::string_view nameOrHex)
        pre(nameOrHex.length() >= 3)
        pre(nameOrHex.length() <= 20)
    {
        if (nameOrHex.starts_with("#")) {
            const auto ln = nameOrHex.length();

            if (ln >= 4 && ln <= 13) {
                std::string_view hex = nameOrHex;
                hex.remove_prefix(1);

                const auto hexLn = hex.length();

                // Now because we have C++26 we can use subview here quickly.
                if (hexLn == 3) {
                    // #R,G,B
                    const auto r = core::JobStringUtils::toUInt64(hex.subview<0, 1>(), 16);
                    const auto g = core::JobStringUtils::toUInt64(hex.subview<1, 1>(), 16);
                    const auto b = core::JobStringUtils::toUInt64(hex.subview<2, 1>(), 16);

                    // Expand R -> RR, etc.
                    return {
                        core::JobColorType::Rgba,
                        (0xFFULL << 24) |
                            ((r * 0x11ULL) << 16) |
                            ((g * 0x11ULL) << 8) |
                            (b * 0x11ULL)
                    };
                } else if (hexLn == 6) {
                    // #RR,GG,BB
                    return {
                        core::JobColorType::Rgba,
                        (0xFFULL << 24) |
                            (core::JobStringUtils::toUInt64(hex.subview<0, 2>(), 16) << 16) |
                            (core::JobStringUtils::toUInt64(hex.subview<2, 2>(), 16) << 8) |
                            core::JobStringUtils::toUInt64(hex.subview<4, 2>(), 16)
                    };
                } else if (hexLn == 8) {
                    // #AA,RR,GG,BB
                    return {
                        core::JobColorType::Rgba,
                        (core::JobStringUtils::toUInt64(hex.subview<0, 2>(), 16) << 24) |
                            (core::JobStringUtils::toUInt64(hex.subview<2, 2>(), 16) << 16) |
                            (core::JobStringUtils::toUInt64(hex.subview<4, 2>(), 16) << 8) |
                            core::JobStringUtils::toUInt64(hex.subview<6, 2>(), 16)
                    };
                } else if (hexLn == 9) {
                    // #RRR,GGG,BBB
                    // Expand 12-bit RGB channels to 16-bit RGBA channels.
                    const auto r = expand12To16(core::JobStringUtils::toUInt64(hex.subview<0, 3>(), 16));
                    const auto g = expand12To16(core::JobStringUtils::toUInt64(hex.subview<3, 3>(), 16));
                    const auto b = expand12To16(core::JobStringUtils::toUInt64(hex.subview<6, 3>(), 16));

                    return {
                        core::JobColorType::Rgba16, (0xFFFFULL << 48) | (r << 32) | (g << 16) | b
                    };
                } else if (hexLn == 12) {
                    // #RRRR,GGGG,BBBB
                    return {
                        core::JobColorType::Rgba16,
                        (0xFFFFULL << 48) |
                            (core::JobStringUtils::toUInt64(hex.subview<0, 4>(), 16) << 32) |
                            (core::JobStringUtils::toUInt64(hex.subview<4, 4>(), 16) << 16) |
                            core::JobStringUtils::toUInt64(hex.subview<8, 4>(), 16)
                    };
                }
            } // fallthrough
        } else {
            // This way it can be Black or black or LightGoldenRodYellow.
            return {
                core::JobColorType::Rgba,
                JobColorKeyword::rgb(core::JobStringUtils::toLower(nameOrHex)).toInt()
            };
        }

        return {
            core::JobColorType::Rgba,
            core::JobRgb{0, 0, 0, 255}.toInt()
        };
    }

private:
    [[nodiscard]] static constexpr std::uint64_t expand12To16(std::uint64_t value) noexcept
    {
        return (value << 4) | (value >> 8);
    }
};

}