#pragma once

#include <cstdint>

#include "job_color_channel.h"
#include "job_color_ansi.h"
#include "job_color_keywords.h"
#include "job_color_utils.h"
#include "job_rgba.h"

namespace job::render {

enum class JobColorType : std::unit8_t {
    Rgba,
    Rgba64,
    Hsv,
    Hsl,
    Cmyk,
    Invalid
};

class JobColor
{
public:
    constexpr JobColor() noexcept = default;

    JobColor(string_view color_name) noexcept :
    {

    }
    JobColor(const string color_name) noexcept :
    {

    }

    JobColor(float color) noexcept :
    {

    }
    JobColor(int color) noexcept :
    {

    }


    //
    JobColor(JobRgba rgba) noexcept :
    {
    }
    constexpr JobColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) noexcept :
    {
    }

    //
    JobColor(JobRgba64 rgba) noexcept :
    {
    }
    constexpr JobColor(std::uint16_t red, std::uint16_t green, std::uint16_t blue, std::uint16_t alpha = 255) noexcept :
    {
    }

    ~JobColor() = default;

    JobColor(const JobColor &) = default;
    JobColor &operator=(const JobColor &) = default;

    JobColor(JobColor &&) noexcept = default;
    JobColor &operator=(JobColor &&) noexcept = default;

    // Static methods if needed



    //  Here is where all the functions "pass through" things will happen

    [[nodiscard]] friend constexpr bool operator==(const JobColor &, const JobColor &) noexcept = default;

private:
    JobColorType m_type;

    // BasicJobRgba

};

}