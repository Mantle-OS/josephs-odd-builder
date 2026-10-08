#pragma once
#include <cstdint>
namespace job::core {
enum class JobColorType : std::uint8_t
{
    Rgba = 0,
    Rgba16,
    Hsv,
    Hsl,
    Hwb,
    Cmyk,
    Ncol,
    Invalid // = 7 slash sentanal
};
}
