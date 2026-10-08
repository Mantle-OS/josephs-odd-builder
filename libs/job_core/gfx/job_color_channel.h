#pragma once

#include <cstdint>

namespace job::core {

enum class JobColorChannel : std::uint8_t
{
    // RGBA[8, 16]
    None,
    Red,
    Green,
    Blue,
    Alpha,

    // HS[LV]
    Hue,
    Saturation,
    Value,
    Lightness,

    // CMYK
    Cyan,
    Magenta,
    Yellow,
    Black,

    // NCol
    Whiteness,
    Blackness,
};

}
