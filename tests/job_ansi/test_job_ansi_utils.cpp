#include <catch2/catch_test_macros.hpp>
#include "utils/job_ansi_utils.h"
#include "utils/job_ansi_suffix.h"

using namespace job::ansi::utils;
using namespace job::ansi::utils::enums;
namespace suffix = job::ansi::utils::suffix;


TEST_CASE("ANSI Sequence Classification", "[utils][ansi][classify]")
{
    SECTION("Plain Text") {
        CHECK(classifyAnsiType("Hello") == AnsiSequenceType::TEXT);
        CHECK(classifyAnsiType(" ") == AnsiSequenceType::TEXT);
    }

    SECTION("CSI Sequences") {
        // Uses suffix::CSI ("\x1B[")
        CHECK(classifyAnsiType(suffix::CSI + "A") == AnsiSequenceType::CSI);
        CHECK(classifyAnsiType(suffix::CSI + "31m") == AnsiSequenceType::CSI);
    }

    SECTION("DECSET Sequences") {
        // Uses suffix::DECSET_ESC ("\x1B[?")
        CHECK(classifyAnsiType(suffix::DECSET_ESC + "25h") == AnsiSequenceType::DECSET);
    }

    SECTION("OSC Sequences") {
        // Uses suffix::OSC ("\x1B]")
        CHECK(classifyAnsiType(suffix::OSC + "0;Title" + suffix::BEL) == AnsiSequenceType::OSC);
    }

    SECTION("ESC Sequences (Single Char)") {
        // Concatenation avoids the hex escape ambiguity
        CHECK(classifyAnsiType(suffix::ESC + "c") == AnsiSequenceType::ESC); // RIS
        CHECK(classifyAnsiType(suffix::ESC + "7") == AnsiSequenceType::ESC); // Save Cursor
    }

    SECTION("Unknown / Malformed") {
        // Just ESC (incomplete)
        CHECK(classifyAnsiType(suffix::ESC) == AnsiSequenceType::UNKNOWN);

        // ESC followed by something weird (length > 2 but not CSI/OSC)
        CHECK(classifyAnsiType(suffix::ESC + " A") == AnsiSequenceType::UNKNOWN);
    }
}

TEST_CASE("Suffix Constants Contract", "[utils][ansi][suffix]")
{
    // Verify the constants match standard expectations
    CHECK(suffix::ESC == "\x1B");
    CHECK(suffix::CSI == "\x1B[");
    CHECK(suffix::OSC == "\x1B]");
    CHECK(suffix::ST  == "\x1B\\");

    // Verify compound constants
    CHECK(suffix::RESET_SGR == "\x1B[0m");
    CHECK(suffix::ENABLE_ALT_SCREEN == "\x1B[?1049h");
}

TEST_CASE("Character Width Calculation", "[utils][ansi][width]")
{
    SECTION("Control Characters (Width 0)") {
        CHECK(charWidth(0x00) == 0); // Null
        CHECK(charWidth(suffix::ESC_KEY) == 0); // ESC (27)
        CHECK(charWidth('\n') == 0);
        CHECK(charWidth(0x7F) == 0); // DEL
    }

    SECTION("Basic Latin (Width 1)") {
        CHECK(charWidth('A') == 1);
        CHECK(charWidth(suffix::SPACEBAR) == 1);
        CHECK(charWidth('~') == 1);
    }

    SECTION("CJK / Wide Characters (Width 2)") {
        CHECK(charWidth(0x1100) == 2); // Hangul Jamo
        CHECK(charWidth(0x4E00) == 2); // CJK Unified Ideograph
        CHECK(charWidth(0xAC00) == 2); // Hangul Syllable
        CHECK(charWidth(0xFF01) == 2); // Fullwidth exclamation mark
    }
}

TEST_CASE("Sequence Termination Checks", "[utils][ansi][terminator]")
{
    SECTION("isFinalChar") {
        CHECK(isFinalChar('@'));
        CHECK(isFinalChar('~'));
        CHECK(isFinalChar('m')); // SGR
        CHECK(isFinalChar('H')); // CUP

        CHECK_FALSE(isFinalChar('?'));
        CHECK_FALSE(isFinalChar(suffix::SPACEBAR));
        CHECK_FALSE(isFinalChar('0'));
    }
}

TEST_CASE("Decoder Functions", "[utils][ansi][decode]")
{
    SECTION("decodeCsiCode") {
        CHECK(decodeCsiCode('A') == CSI_CODE::CUU);
        CHECK(decodeCsiCode('m') == CSI_CODE::SGR);
        CHECK(decodeCsiCode('?') == CSI_CODE::UNKNOWN);
    }

    SECTION("toChar (Reverse CSI)") {
        CHECK(toChar(CSI_CODE::CUU) == 'A');
        CHECK(toChar(CSI_CODE::SGR) == 'm');
    }

    SECTION("decodeEscCode") {
        CHECK(decodeEscCode('c') == ESC_CODE::RESET_TO_INITIAL);
        CHECK(decodeEscCode('(') == ESC_CODE::SELECT_CHARSET_G0);
    }

    SECTION("decodeDecsetCode") {
        CHECK(decodeDecsetCode(1)    == DECSET_PRIVATE_CODE::CURSOR_KEYS);
        CHECK(decodeDecsetCode(25)   == DECSET_PRIVATE_CODE::CURSOR_VISIBLE);
        CHECK(decodeDecsetCode(1006) == DECSET_PRIVATE_CODE::MOUSE_SGR_EXT);
    }

    SECTION("decodeCharset") {
        CHECK(decodeCharset('B') == CharsetDesignator::US_ASCII);
        CHECK(decodeCharset('0') == CharsetDesignator::DEC_SPECIAL_GRAPHICS);
    }
}
