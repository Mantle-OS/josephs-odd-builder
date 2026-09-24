#pragma once

#include <cstdint>
#include <optional>

#include <job_render_format.h>
#include <job_render_format_type.h>

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

    [[nodiscard]] static constexpr bool supports(FormatType format) noexcept
    {
        if (format == FormatType::Unknown)
            return false;

        const auto info = JobRenderFormat::info(format);

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

    [[nodiscard]] static constexpr std::optional<Packed> pack(JobColor color, FormatType format) noexcept
    {
        if (!supports(format))
            return std::nullopt;

        const auto info = JobRenderFormat::info(format);

        Packed packed{};

        for (const auto &channel : info.channels) {
            if (!channel.valid())
                continue;

            const auto source = color.channel(channel.channel);
            const auto scaled = scale(source, channel.bits);

            packed |= scaled << channel.shift;
        }

        return packed;
    }

private:
    [[nodiscard]] static constexpr Packed scale(JobColor::Channel value, std::uint8_t bits) noexcept
    {
        if (bits == 0)
            return 0;

        if (bits == 8)
            return static_cast<Packed>(value);

        if (bits >= 64)
            return 0;

        const Packed maximum = (Packed{1} << bits) - 1;

        return (static_cast<Packed>(value) * maximum + 127u) / 255u;
    }
};

}