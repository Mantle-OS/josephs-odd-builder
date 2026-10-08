#pragma once

#include <cstdint>
#include <optional>

#include <job_render_format.h>
#include <job_render_format_type.h>
#include <job_color_concept.h>
#include <job_rgb.h>
#include "job_color.h"

namespace job::render {

class JobColorCodec
{
public:
    using Packed = std::uint64_t;

    JobColorCodec() = delete;
    ~JobColorCodec() = delete;

    JobColorCodec(const JobColorCodec &) = delete;
    JobColorCodec &operator=(const JobColorCodec &) = delete;

    JobColorCodec(JobColorCodec &&) = delete;
    JobColorCodec &operator=(JobColorCodec &&) = delete;

    [[nodiscard]] static constexpr bool supports(core::FormatType format) noexcept
    {
        if (format == core::FormatType::Unknown)
            return false;

        const auto info = core::JobRenderFormat::info(format);
        if (info.planeCount != 1)
            return false;

        if (info.bitsPerPixel == 0 || info.bitsPerPixel > 64)
            return false;

        if (info.yuv || info.floatingPoint || info.colorIndexed)
            return false;

        for (const auto &channel : info.channels) {
            if (channel.valid())
                return true;
        }

        return false;
    }

    template <core::JobColorConcept Color>
    [[nodiscard]] static constexpr std::optional<Packed> pack(const Color &color, core::FormatType format) noexcept
    {
        if (!supports(format))
            return std::nullopt;

        if (!color.supports(format))
            return std::nullopt;

        const auto info = core::JobRenderFormat::info(format);
        Packed packed{};
        for (const auto &channel : info.channels) {
            if (!channel.valid())
                continue;

            const auto scaled = scale(color, channel.channel, channel.bits);
            packed |= scaled << channel.shift;
        }

        return packed;
    }

private:
    template <core::JobColorConcept Color>
    [[nodiscard]] static constexpr Packed scale(const Color &color, core::JobColorChannel channel, std::uint8_t bits) noexcept
    {
        if (bits == 0 || bits >= 64)
            return 0;

        const auto value = color.channel(channel);
        const auto sourceMaximum = color.channelMaximum(channel);

        if (sourceMaximum == 0)
            return 0;

        const Packed destinationMaximum = (Packed{1} << bits) - 1;
        return (static_cast<Packed>(value) * destinationMaximum + static_cast<Packed>(sourceMaximum / 2)) / static_cast<Packed>(sourceMaximum);
    }
};

}