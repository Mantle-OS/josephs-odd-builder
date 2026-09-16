#pragma once

#include <string_view>
#include "job_ansi_enums.h"

namespace job::ansi::utils {

using namespace job::ansi::utils::enums;

inline int charWidth(char32_t ch)
{
    // Treat control chars as width 0
    if (ch < 0x20 || (ch >= 0x7f && ch < 0xa0))
        return 0;

    // Basic CJK ranges
    if ((ch >= 0x1100 && ch <= 0x115F) || // Hangul Jamo
        (ch >= 0x2E80 && ch <= 0xA4CF) || // CJK
        (ch >= 0xAC00 && ch <= 0xD7A3) || // Hangul Syllables
        (ch >= 0xF900 && ch <= 0xFAFF) || // CJK Compatibility Ideographs
        (ch >= 0xFE10 && ch <= 0xFE19) || // Vertical Forms
        (ch >= 0xFF01 && ch <= 0xFF60))   // Full-width forms
        return 2;

    return 1;
}

// Primary Device Attributes (DA1)
#ifndef DA1_RESPONSE_OVERRIDE
    inline constexpr const char *DA1_RESPONSE = "\033[?1;2c";
#else
    inline constexpr const char *DA1_RESPONSE = DA1_RESPONSE_OVERRIDE;
#endif

// Secondary Device Attributes (DA2)
#ifndef DA2_RESPONSE_OVERRIDE
    inline constexpr const char *DA2_RESPONSE = "\033[>0;95;0c";
#else
    inline constexpr const char *DA2_RESPONSE = DA2_RESPONSE_OVERRIDE;
#endif

// Tertiary Device Attributes (DA3)
#ifndef DA3_RESPONSE_OVERRIDE
    inline constexpr const char *DA3_RESPONSE = "\033P!|00000000\033\\";
#else
    inline constexpr const char *DA3_RESPONSE = DA3_RESPONSE_OVERRIDE;
#endif

// Escape sequence classification
[[nodiscard]] bool isCsiTerminator(char ch);
[[nodiscard]] bool isEscFinal(char ch);
[[nodiscard]] bool isOscTerminator(const std::string &buffer);
[[nodiscard]] bool isStSequence(const std::string &buffer);

inline AnsiSequenceType classifyAnsiType(std::string_view buf)
{
    if (buf.starts_with("\x1B[?"))
        return AnsiSequenceType::DECSET;

    if (buf.starts_with("\x1B["))
        return AnsiSequenceType::CSI;

    if (buf.starts_with("\x1B]"))
        return AnsiSequenceType::OSC;

    if (buf.starts_with("\x1B") && buf.length() == 2)
        return AnsiSequenceType::ESC;

    if (!buf.starts_with("\x1B"))
        return AnsiSequenceType::TEXT;

    return AnsiSequenceType::UNKNOWN;
}

inline CharsetDesignator decodeCharset(char c)
{
    switch (c) {
    case 'A':
        return CharsetDesignator::UK;
    case 'B':
        return CharsetDesignator::US_ASCII;
    case '0':
        return CharsetDesignator::DEC_SPECIAL_GRAPHICS;
    case '<':
        return CharsetDesignator::DEC_SUPPLEMENTAL;
    case '>':
        return CharsetDesignator::DEC_TECHNICAL;
    case 'C':
        return CharsetDesignator::ISO_LATIN_1;
    case 'D':
        return CharsetDesignator::ISO_LATIN_2;
    default:
        return CharsetDesignator::UNUSED;
    }
}

inline ESC_CODE decodeEscCode(char c)
{
    switch (c) {
    case '(':
        return ESC_CODE::SELECT_CHARSET_G0;
    case ')':
        return ESC_CODE::SELECT_CHARSET_G1;
    case '*':
        return ESC_CODE::SELECT_CHARSET_G2;
    case '+':
        return ESC_CODE::SELECT_CHARSET_G3;
    case '=':
        return ESC_CODE::ENABLE_APP_KEYPAD;
    case '>':
        return ESC_CODE::DISABLE_APP_KEYPAD;
    case 'c':
        return ESC_CODE::RESET_TO_INITIAL;
    case '7':
        return ESC_CODE::SAVE_CURSOR;
    case '8':
        return ESC_CODE::RESTORE_CURSOR;
    case 'Z':
        return ESC_CODE::IDENTIFY_TERMINAL;
    case 'D':
        return ESC_CODE::INDEX;
    case 'E':
        return ESC_CODE::NEXT_LINE;
    case 'H':
        return ESC_CODE::SET_TAB_STOP;
    case 'M':
        return ESC_CODE::REVERSE_INDEX;
    default:
        return ESC_CODE::UNKNOWN;
    }
}

inline DECSET_PRIVATE_CODE decodeDecsetCode(int code)
{
    switch (code) {
    case 1:
        return DECSET_PRIVATE_CODE::CURSOR_KEYS;
    case 3:
        return DECSET_PRIVATE_CODE::COLUMN_MODE_132;
    case 4:
        return DECSET_PRIVATE_CODE::SMOOTH_SCROLL;
    case 5:
        return DECSET_PRIVATE_CODE::REVERSE_VIDEO;
    case 6:
        return DECSET_PRIVATE_CODE::ORIGIN_MODE;
    case 7:
        return DECSET_PRIVATE_CODE::WRAPAROUND_MODE;
    case 8:
        return DECSET_PRIVATE_CODE::AUTOREPEAT_MODE;
    case 9:
        return DECSET_PRIVATE_CODE::INTERLACE_MODE;
    case 12:
        return DECSET_PRIVATE_CODE::BLINKING_CURSOR;
    case 25:
        return DECSET_PRIVATE_CODE::CURSOR_VISIBLE;
    case 1000:
        return DECSET_PRIVATE_CODE::MOUSE_X10;
    case 1001:
        return DECSET_PRIVATE_CODE::MOUSE_VT200_HIGHLIGHT;
    case 1002:
        return DECSET_PRIVATE_CODE::MOUSE_BTN_EVENT;
    case 1003:
        return DECSET_PRIVATE_CODE::MOUSE_ANY_EVENT;
    case 1004:
        return DECSET_PRIVATE_CODE::FOCUS_EVENT;
    case 1005:
        return DECSET_PRIVATE_CODE::MOUSE_UTF8;
    case 1006:
        return DECSET_PRIVATE_CODE::MOUSE_SGR_EXT;
    case 1007:
        return DECSET_PRIVATE_CODE::MOUSE_ALTERNATE_SCROLL;
    case 2004:
        return DECSET_PRIVATE_CODE::BRACKETED_PASTE;
    case 1047:
        return DECSET_PRIVATE_CODE::ALTERNATE_SCREEN;
    case 1048:
        return DECSET_PRIVATE_CODE::SAVE_RESTORE_CURSOR;
    case 1049:
        return DECSET_PRIVATE_CODE::ALTERNATE_SCREEN_EXT;
    default:
        return DECSET_PRIVATE_CODE::UNKNOWN;
    }
}

inline CSI_CODE decodeCsiCode(char ch)
{
    switch (ch) {
    case '@':
        return CSI_CODE::ICH;
    case 'A':
        return CSI_CODE::CUU;
    case 'B':
        return CSI_CODE::CUD;
    case 'C':
        return CSI_CODE::CUF;
    case 'D':
        return CSI_CODE::CUB;
    case 'E':
        return CSI_CODE::CNL;
    case 'F':
        return CSI_CODE::CPL;
    case 'G':
        return CSI_CODE::CHA;
    case 'H':
        return CSI_CODE::CUP;
    case 'I':
        return CSI_CODE::CHT;
    case 'J':
        return CSI_CODE::ED;
    case 'K':
        return CSI_CODE::EL;
    case 'L':
        return CSI_CODE::IL;
    case 'M':
        return CSI_CODE::DL;
    case 'N':
        return CSI_CODE::EF;
    case 'O':
        return CSI_CODE::EA;
    case 'P':
        return CSI_CODE::DCH;
    case 'Q':
        return CSI_CODE::SEE;
    case 'R':
        return CSI_CODE::CPR;
    case 'S':
        return CSI_CODE::SU;
    case 'T':
        return CSI_CODE::SD;
    case 'U':
        return CSI_CODE::NP;
    case 'V':
        return CSI_CODE::PP;
    case 'W':
        return CSI_CODE::CTC;
    case 'X':
        return CSI_CODE::ECH;
    case 'Z':
        return CSI_CODE::CBT;
    case 'a':
        return CSI_CODE::HPR;
    case 'b':
        return CSI_CODE::REP;
    case 'c':
        return CSI_CODE::DA;
    case 'd':
        return CSI_CODE::VPA;
    case 'f':
        return CSI_CODE::HVP;
    case 'g':
        return CSI_CODE::TBC;
    case 'h':
        return CSI_CODE::SM;
    case 'i':
        return CSI_CODE::MC;
    case 'l':
        return CSI_CODE::RM;
    case 'm':
        return CSI_CODE::SGR;
    case 'n':
        return CSI_CODE::DSR;
    case 'p':
        return CSI_CODE::DECLL;
    case 'q':
        return CSI_CODE::DECSCUSR;
    case 'r':
        return CSI_CODE::DECSTBM;
    case 's':
        return CSI_CODE::SCP;
    case 't':
        return CSI_CODE::WINDOW_MANIP;
    case 'u':
        return CSI_CODE::RCP;
    case 'v':
        return CSI_CODE::DECDWL;
    case 'w':
        return CSI_CODE::DECSWL;
    case 'z':
        return CSI_CODE::DECFRA;
    case '{':
        return CSI_CODE::PASTE_BEGIN;
    case '|':
        return CSI_CODE::PASTE_END;
    case '"':
        return CSI_CODE::DECSCA;
    case '#':
        return CSI_CODE::DECCRA;
    case '3':
        return CSI_CODE::DECDHL_TOP;
    case '4':
        return CSI_CODE::DECDHL_BOTTOM;
    case '=':
        return CSI_CODE::DA3;
    case '>':
        return CSI_CODE::DA2;
    case 'Y':
        return CSI_CODE::SR;
    default:
        return CSI_CODE::UNKNOWN;
    }
}

inline char toChar(CSI_CODE code)
{
    switch (code) {
    case CSI_CODE::ICH:
        return '@';
    case CSI_CODE::CUU:
        return 'A';
    case CSI_CODE::CUD:
        return 'B';
    case CSI_CODE::CUF:
        return 'C';
    case CSI_CODE::CUB:
        return 'D';
    case CSI_CODE::CNL:
        return 'E';
    case CSI_CODE::CPL:
        return 'F';
    case CSI_CODE::CHA:
        return 'G';
    case CSI_CODE::CUP:
        return 'H';
    case CSI_CODE::CHT:
        return 'I';
    case CSI_CODE::ED:
        return 'J';
    case CSI_CODE::EL:
        return 'K';
    case CSI_CODE::IL:
        return 'L';
    case CSI_CODE::DL:
        return 'M';
    case CSI_CODE::EF:
        return 'N';
    case CSI_CODE::EA:
        return 'O';
    case CSI_CODE::DCH:
        return 'P';
    case CSI_CODE::SEE:
        return 'Q';
    case CSI_CODE::CPR:
        return 'R';
    case CSI_CODE::SU:
        return 'S';
    case CSI_CODE::SD:
        return 'T';
    case CSI_CODE::NP:
        return 'U';
    case CSI_CODE::PP:
        return 'V';
    case CSI_CODE::CTC:
        return 'W';
    case CSI_CODE::ECH:
        return 'X';
    case CSI_CODE::CBT:
        return 'Z';
    case CSI_CODE::HPR:
        return 'a';
    case CSI_CODE::REP:
        return 'b';
    case CSI_CODE::DA:
        return 'c';
    case CSI_CODE::VPA:
        return 'd';
    case CSI_CODE::HVP:
        return 'f';
    case CSI_CODE::TBC:
        return 'g';
    case CSI_CODE::SM:
        return 'h';
    case CSI_CODE::MC:
        return 'i';
    case CSI_CODE::RM:
        return 'l';
    case CSI_CODE::SGR:
        return 'm';
    case CSI_CODE::DSR:
        return 'n';
    case CSI_CODE::DECLL:
        return 'p';
    case CSI_CODE::DECSCUSR:
        return 'q';
    case CSI_CODE::DECSTBM:
        return 'r';
    case CSI_CODE::SCP:
        return 's';
    case CSI_CODE::WINDOW_MANIP:
        return 't';
    case CSI_CODE::RCP:
        return 'u';
    case CSI_CODE::DECDWL:
        return 'v';
    case CSI_CODE::DECSWL:
        return 'w';
    case CSI_CODE::DECFRA:
        return 'z';
    case CSI_CODE::PASTE_BEGIN:
        return '{';
    case CSI_CODE::PASTE_END:
        return '|';
    case CSI_CODE::DECSCA:
        return '"';
    case CSI_CODE::DECCRA:
        return '#';
    case CSI_CODE::DECDHL_TOP:
        return '3';
    case CSI_CODE::DECDHL_BOTTOM:
        return '4';
    case CSI_CODE::DA3:
        return '=';
    case CSI_CODE::DA2:
        return '>';
    case CSI_CODE::SR:
        return 'Y';
    default:
        return '\0';
    }
}

inline bool isFinalChar(char ch)
{
    return (ch >= '@' && ch <= '~');
}

}

