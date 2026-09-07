#pragma once
#include <contracts>
#include <string_view>

// Originally generated from the YAML 1.2 grammar using for_science.py,
// then manually reduced and updated for job_yaml.
// Source history:
// https://github.com/JosephMillsAtWork/yaml-grammar

namespace job::yaml {

// ========================================
// Literal characters
// ========================================

inline constexpr char SequenceEntry = '-';
inline constexpr char MappingKey = '?';
inline constexpr char MappingValue = ':';
inline constexpr char CollectEntry = ',';
inline constexpr char SequenceStart = '[';
inline constexpr char SequenceEnd = ']';
inline constexpr char MappingStart = '{';
inline constexpr char MappingEnd = '}';
inline constexpr char Comment = '#';
inline constexpr char Anchor = '&';
inline constexpr char Alias = '*';
inline constexpr char Tag = '!';
inline constexpr char Literal = '|';
inline constexpr char Folded = '>';
inline constexpr char SingleQuote = '\'';
inline constexpr char DoubleQuote = '"';
inline constexpr char Directive = '%';
inline constexpr char Escape = '\\';
inline constexpr char EscNull = '0';
inline constexpr char EscBell = 'a';
inline constexpr char EscBackspace = 'b';
inline constexpr char EscLineFeed = 'n';
inline constexpr char EscVerticalTab = 'v';
inline constexpr char EscFormFeed = 'f';
inline constexpr char EscCarriageReturn = 'r';
inline constexpr char EscEscape = 'e';
inline constexpr char EscDoubleQuote = '"';
inline constexpr char EscSlash = '/';
inline constexpr char EscBackslash = '\\';
inline constexpr char EscNextLine = 'N';
inline constexpr char EscNonBreakingSpace = '_';
inline constexpr char EscLineSeparator = 'L';
inline constexpr char EscParagraphSeparator = 'P';
inline constexpr char PrimaryTagHandle = '!';
inline constexpr char NonSpecificTag = '!';

// ========================================
// Code points
// ========================================

inline constexpr char32_t ByteOrderMark = U'\uFEFF';
inline constexpr char32_t LineFeed = U'\n';
inline constexpr char32_t CarriageReturn = U'\r';
inline constexpr char Space = ' ';
inline constexpr char32_t Tab = U'\t';
inline constexpr char EscSpace = ' ';

// ========================================
// Literal strings
// ========================================

inline constexpr std::string_view SecondaryTagHandle = "!!";
inline constexpr std::string_view QuotedQuote = "''";
inline constexpr std::string_view DirectivesEnd = "---";
inline constexpr std::string_view DocumentEnd = "...";

// ========================================
// Character ranges
// ========================================

constexpr bool isNsDecDigit(char32_t c) noexcept
{
    return c >= U'0' && c <= U'9';
}

// ========================================
// Character predicates
// ========================================

constexpr bool isCPrintable(char32_t c) noexcept
{
    return c == Tab ||
           c == LineFeed ||
           c == CarriageReturn ||
           (c >= 0x20 && c <= 0x7E) ||
           c == 0x85 ||
           (c >= 0xA0 && c <= 0xD7FF) ||
           (c >= 0xE000 && c <= 0xFFFD) ||
           (c >= 0x10000 && c <= 0x10FFFF);
}

constexpr bool isNbJson(char32_t c) noexcept
{
    return c == Tab || (c >= 0x20 && c <= 0x10FFFF);
}

constexpr bool isCReserved(char32_t c) noexcept
{
    return c == '@' || c == '`';
}

constexpr bool isCIndicator(char32_t c) noexcept
{
    return c == SequenceEntry ||
           c == MappingKey ||
           c == MappingValue ||
           c == CollectEntry ||
           c == SequenceStart ||
           c == SequenceEnd ||
           c == MappingStart ||
           c == MappingEnd ||
           c == Comment ||
           c == Anchor ||
           c == Alias ||
           c == Tag ||
           c == Literal ||
           c == Folded ||
           c == SingleQuote ||
           c == DoubleQuote ||
           c == Directive ||
           c == '@' ||
           c == '`';
}

constexpr bool isCFlowIndicator(char32_t c) noexcept
{
    return c == CollectEntry ||
           c == SequenceStart ||
           c == SequenceEnd ||
           c == MappingStart ||
           c == MappingEnd;
}

constexpr bool isBChar(char32_t c) noexcept
{
    return c == LineFeed || c == CarriageReturn;
}

constexpr bool isNbChar(char32_t c) noexcept
{
    return isCPrintable(c) && !isBChar(c) && c != ByteOrderMark;
}

constexpr bool isSWhite(char32_t c) noexcept
{
    return c == Space || c == Tab;
}

constexpr bool isNsChar(char32_t c) noexcept
{
    return isNbChar(c) && !isSWhite(c);
}

constexpr bool isNsHexDigit(char32_t c) noexcept
{
    return isNsDecDigit(c) ||
           (c >= U'A' && c <= U'F') ||
           (c >= U'a' && c <= U'f');
}

constexpr bool isNsAsciiLetter(char32_t c) noexcept
{
    return (c >= U'A' && c <= U'Z') ||
           (c >= U'a' && c <= U'z');
}

constexpr bool isNsWordChar(char32_t c) noexcept
{
    return isNsDecDigit(c) || isNsAsciiLetter(c) || c == SequenceEntry;
}

constexpr bool isNsEscHorizontalTab(char32_t c) noexcept
{
    return c == 't' || c == Tab;
}

constexpr bool isNsAnchorChar(char32_t c) noexcept
{
    return isNsChar(c) && !isCFlowIndicator(c);
}

constexpr bool isNsPlainSafeIn(char32_t c) noexcept
{
    return isNsChar(c) && !isCFlowIndicator(c);
}

} // namespace job::yaml