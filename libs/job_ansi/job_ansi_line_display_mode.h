#pragma once

namespace job::ansi {
// Line display modes for DECSWL, DECDWL, DECDHL
enum class LineDisplayMode {
    SingleWidth,         // Normal width (DECSWL)
    DoubleWidth,        // Double width (DECDWL)
    DoubleWidthTop,     // Double width, top half (DECDHL)
    DoubleWidthBottom,  // Double width, bottom half (DECDHL)
    DoubleHeight,       // Double height (DECDHL)
    DoubleHeightTop,    // Double height, top half (DECDHL)
    DoubleHeightBottom  // Double height, bottom half (DECDHL)
};
}

