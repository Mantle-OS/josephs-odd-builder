#pragma once

#include <cstdint>

#include "job_render_format_type.h"

namespace job::render {

enum class FbFormatSource : std::uint8_t
{
    Unknown,
    Legacy,
    FourCc
};

struct JobFbFormatResult
{
    core::FormatType    format{core::FormatType::Unknown};
    FbFormatSource      source{FbFormatSource::Unknown};

    using FourCc = std::uint32_t;
    FourCc          fourCc{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return format != core::FormatType::Unknown;
    }

    [[nodiscard]] constexpr bool legacy() const noexcept
    {
        return source == FbFormatSource::Legacy;
    }

    [[nodiscard]] constexpr bool fourCcFormat() const noexcept
    {
        return source == FbFormatSource::FourCc;
    }

    bool operator==(const JobFbFormatResult &) const noexcept = default;
};

}