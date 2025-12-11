#pragma once
#include <cstdint>

namespace ansipp::tui::base {

struct Style {
    uint32_t fg = 0xFFFFFFFF;
    uint32_t bg = 0x00000000;
    bool bold = false;
    bool underline = false;
    // padding, margins, border styles later
};

} // namespace ansipp
