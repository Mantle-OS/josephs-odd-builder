#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "utils/job_ansi_enums.h"

using namespace job::ansi::utils::enums;

TEST_CASE("CSI Code Value Contract", "[enums][csi]")
{
    CHECK(static_cast<char>(CSI_CODE::CUP) == 'H'); // Cursor Position
    CHECK(static_cast<char>(CSI_CODE::SGR) == 'm'); // Select Graphic Rendition
    CHECK(static_cast<char>(CSI_CODE::ED)  == 'J'); // Erase in Display
    CHECK(static_cast<char>(CSI_CODE::EL)  == 'K'); // Erase in Line
    CHECK(static_cast<char>(CSI_CODE::DA)  == 'c'); // Device Attributes
}

TEST_CASE("SGR Code Value Contract", "[enums][sgr]")
{
    // 0 is Reset
    CHECK(static_cast<int>(SGR_CODE::RESET) == 0);

    // 1 is Bold
    CHECK(static_cast<int>(SGR_CODE::BOLD) == 1);

    // Standard Foreground (30-37)
    CHECK(static_cast<int>(SGR_CODE::FG_BLACK)   == 30);
    CHECK(static_cast<int>(SGR_CODE::FG_RED)     == 31);
    CHECK(static_cast<int>(SGR_CODE::FG_WHITE)   == 37);

    // Standard Background (40-47)
    CHECK(static_cast<int>(SGR_CODE::BG_BLACK)   == 40);
    CHECK(static_cast<int>(SGR_CODE::BG_WHITE)   == 47);

    // Extended (256/TrueColor)
    CHECK(static_cast<int>(SGR_CODE::EXTENDED_FG) == 38);
    CHECK(static_cast<int>(SGR_CODE::EXTENDED_BG) == 48);
}

TEST_CASE("DEC Mode Private Constants", "[enums][decset]")
{
    // These are the "?1049h" style numbers
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::CURSOR_VISIBLE)      == 25);
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::ALTERNATE_SCREEN)    == 1047);
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::ALTERNATE_SCREEN_EXT) == 1049);
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::BRACKETED_PASTE)     == 2004);

    // Mouse Modes
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::MOUSE_BTN_EVENT) == 1002);
    CHECK(static_cast<int>(DECSET_PRIVATE_CODE::MOUSE_SGR_EXT)   == 1006);
}

TEST_CASE("OSC Code Constants", "[enums][osc]")
{
    // \x1B]0;Title\x07
    CHECK(static_cast<int>(OSC_CODE::WINDOW_ICON) == 0);
    CHECK(static_cast<int>(OSC_CODE::WINDOW_TITLE2) == 2);

    // \x1B]52;...
    CHECK(static_cast<int>(OSC_CODE::SET_CLIPBOARD) == 52);
}


TEST_CASE("Enum Type Safety", "[enums][types]")
{
    // Verify underlying types to ensure ABI compatibility if needed
    CHECK(std::is_same_v<std::underlying_type_t<CSI_CODE>, char>);
    CHECK(std::is_same_v<std::underlying_type_t<SGR_CODE>, int>);
    CHECK(std::is_same_v<std::underlying_type_t<ESC_CODE>, char>);
}

TEST_CASE("Cursor Style Enum", "[enums][cursor]")
{
    // These match DECSCUSR (CSI Ps SP q)
    CHECK(static_cast<int>(CursorStyle::BlinkingBlock) == 1); // or 1, strictly 0 is user-config but usually block
    CHECK(static_cast<int>(CursorStyle::SteadyBlock)   == 2);
    CHECK(static_cast<int>(CursorStyle::SteadyBar)     == 6);
}
