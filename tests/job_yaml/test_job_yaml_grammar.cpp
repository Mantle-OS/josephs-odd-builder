#include <catch2/catch_test_macros.hpp>

#include <job_yaml_grammar.h>

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Literal characters
// ========================================

static_assert(SequenceEntry == '-');
static_assert(MappingKey == '?');
static_assert(MappingValue == ':');
static_assert(CollectEntry == ',');
static_assert(SequenceStart == '[');
static_assert(SequenceEnd == ']');
static_assert(MappingStart == '{');
static_assert(MappingEnd == '}');
static_assert(Comment == '#');
static_assert(Anchor == '&');
static_assert(Alias == '*');
static_assert(Tag == '!');
static_assert(Literal == '|');
static_assert(Folded == '>');
static_assert(SingleQuote == '\'');
static_assert(DoubleQuote == '"');
static_assert(Directive == '%');
static_assert(Escape == '\\');

static_assert(EscNull == '0');
static_assert(EscBell == 'a');
static_assert(EscBackspace == 'b');
static_assert(EscLineFeed == 'n');
static_assert(EscVerticalTab == 'v');
static_assert(EscFormFeed == 'f');
static_assert(EscCarriageReturn == 'r');
static_assert(EscEscape == 'e');
static_assert(EscDoubleQuote == '"');
static_assert(EscSlash == '/');
static_assert(EscBackslash == '\\');
static_assert(EscNextLine == 'N');
static_assert(EscNonBreakingSpace == '_');
static_assert(EscLineSeparator == 'L');
static_assert(EscParagraphSeparator == 'P');

static_assert(PrimaryTagHandle == '!');
static_assert(NonSpecificTag == '!');

// ========================================
// Code points
// ========================================

static_assert(ByteOrderMark == U'\uFEFF');
static_assert(LineFeed == U'\n');
static_assert(CarriageReturn == U'\r');
static_assert(Space == ' ');
static_assert(Tab == U'\t');
static_assert(EscSpace == ' ');

// ========================================
// Literal strings
// ========================================

static_assert(SecondaryTagHandle == "!!"sv);
static_assert(QuotedQuote == "''"sv);
static_assert(DirectivesEnd == "---"sv);
static_assert(DocumentEnd == "..."sv);

// ========================================
// Character ranges
// ========================================

static_assert(isNsDecDigit(U'0'));
static_assert(isNsDecDigit(U'5'));
static_assert(isNsDecDigit(U'9'));
static_assert(!isNsDecDigit(U'/'));
static_assert(!isNsDecDigit(U':'));
static_assert(!isNsDecDigit(U'a'));

// ========================================
// Printable characters
// ========================================

static_assert(isCPrintable(0x09));
static_assert(isCPrintable(0x0A));
static_assert(isCPrintable(0x0D));

static_assert(!isCPrintable(0x00));
static_assert(!isCPrintable(0x08));
static_assert(!isCPrintable(0x0B));
static_assert(!isCPrintable(0x0C));
static_assert(!isCPrintable(0x0E));

static_assert(isCPrintable(0x20));
static_assert(isCPrintable(0x7E));
static_assert(!isCPrintable(0x7F));

static_assert(isCPrintable(0x85));
static_assert(!isCPrintable(0x86));

static_assert(isCPrintable(0xA0));
static_assert(isCPrintable(0xD7FF));
static_assert(!isCPrintable(0xD800));
static_assert(!isCPrintable(0xDFFF));
static_assert(isCPrintable(0xE000));
static_assert(isCPrintable(0xFFFD));
static_assert(!isCPrintable(0xFFFE));
static_assert(!isCPrintable(0xFFFF));

static_assert(isCPrintable(0x10000));
static_assert(isCPrintable(0x10FFFF));
static_assert(!isCPrintable(0x110000));

// ========================================
// JSON characters
// ========================================

static_assert(isNbJson(Tab));
static_assert(isNbJson(U' '));
static_assert(isNbJson(U'~'));
static_assert(isNbJson(0x80));
static_assert(isNbJson(0x10FFFF));

static_assert(!isNbJson(0x00));
static_assert(!isNbJson(LineFeed));
static_assert(!isNbJson(CarriageReturn));
static_assert(!isNbJson(0x1F));
static_assert(!isNbJson(0x110000));

// ========================================
// Reserved indicators
// ========================================

static_assert(isCReserved(U'@'));
static_assert(isCReserved(U'`'));

static_assert(!isCReserved(U'!'));
static_assert(!isCReserved(U'#'));
static_assert(!isCReserved(U'a'));

// ========================================
// Indicators
// ========================================

static_assert(isCIndicator(SequenceEntry));
static_assert(isCIndicator(MappingKey));
static_assert(isCIndicator(MappingValue));
static_assert(isCIndicator(CollectEntry));
static_assert(isCIndicator(SequenceStart));
static_assert(isCIndicator(SequenceEnd));
static_assert(isCIndicator(MappingStart));
static_assert(isCIndicator(MappingEnd));
static_assert(isCIndicator(Comment));
static_assert(isCIndicator(Anchor));
static_assert(isCIndicator(Alias));
static_assert(isCIndicator(Tag));
static_assert(isCIndicator(Literal));
static_assert(isCIndicator(Folded));
static_assert(isCIndicator(SingleQuote));
static_assert(isCIndicator(DoubleQuote));
static_assert(isCIndicator(Directive));
static_assert(isCIndicator(U'@'));
static_assert(isCIndicator(U'`'));

static_assert(!isCIndicator(U'a'));
static_assert(!isCIndicator(U'0'));
static_assert(!isCIndicator(U' '));
static_assert(!isCIndicator(U'_'));

// ========================================
// Flow indicators
// ========================================

static_assert(isCFlowIndicator(CollectEntry));
static_assert(isCFlowIndicator(SequenceStart));
static_assert(isCFlowIndicator(SequenceEnd));
static_assert(isCFlowIndicator(MappingStart));
static_assert(isCFlowIndicator(MappingEnd));

static_assert(!isCFlowIndicator(SequenceEntry));
static_assert(!isCFlowIndicator(MappingKey));
static_assert(!isCFlowIndicator(MappingValue));
static_assert(!isCFlowIndicator(Comment));

// ========================================
// Break characters
// ========================================

static_assert(isBChar(LineFeed));
static_assert(isBChar(CarriageReturn));

static_assert(!isBChar(Tab));
static_assert(!isBChar(Space));
static_assert(!isBChar(U'a'));

// ========================================
// Non-break characters
// ========================================

static_assert(isNbChar(U'a'));
static_assert(isNbChar(U'0'));
static_assert(isNbChar(Space));
static_assert(isNbChar(Tab));
static_assert(isNbChar(0x85));

static_assert(!isNbChar(LineFeed));
static_assert(!isNbChar(CarriageReturn));
static_assert(!isNbChar(ByteOrderMark));
static_assert(!isNbChar(0x00));

// ========================================
// White characters
// ========================================

static_assert(isSWhite(Space));
static_assert(isSWhite(Tab));

static_assert(!isSWhite(LineFeed));
static_assert(!isSWhite(CarriageReturn));
static_assert(!isSWhite(U'a'));

// ========================================
// Non-space characters
// ========================================

static_assert(isNsChar(U'a'));
static_assert(isNsChar(U'0'));
static_assert(isNsChar(U'-'));
static_assert(isNsChar(U':'));
static_assert(isNsChar(0x85));

static_assert(!isNsChar(Space));
static_assert(!isNsChar(Tab));
static_assert(!isNsChar(LineFeed));
static_assert(!isNsChar(CarriageReturn));
static_assert(!isNsChar(ByteOrderMark));

// ========================================
// Hexadecimal digits
// ========================================

static_assert(isNsHexDigit(U'0'));
static_assert(isNsHexDigit(U'9'));
static_assert(isNsHexDigit(U'A'));
static_assert(isNsHexDigit(U'F'));
static_assert(isNsHexDigit(U'a'));
static_assert(isNsHexDigit(U'f'));

static_assert(!isNsHexDigit(U'G'));
static_assert(!isNsHexDigit(U'g'));
static_assert(!isNsHexDigit(U'/'));
static_assert(!isNsHexDigit(U':'));

// ========================================
// ASCII letters
// ========================================

static_assert(isNsAsciiLetter(U'A'));
static_assert(isNsAsciiLetter(U'Z'));
static_assert(isNsAsciiLetter(U'a'));
static_assert(isNsAsciiLetter(U'z'));

static_assert(!isNsAsciiLetter(U'0'));
static_assert(!isNsAsciiLetter(U'@'));
static_assert(!isNsAsciiLetter(U'['));
static_assert(!isNsAsciiLetter(U'`'));
static_assert(!isNsAsciiLetter(U'{'));

// ========================================
// Word characters
// ========================================

static_assert(isNsWordChar(U'A'));
static_assert(isNsWordChar(U'z'));
static_assert(isNsWordChar(U'0'));
static_assert(isNsWordChar(U'9'));
static_assert(isNsWordChar(U'-'));

static_assert(!isNsWordChar(U'_'));
static_assert(!isNsWordChar(U'.'));
static_assert(!isNsWordChar(U':'));
static_assert(!isNsWordChar(U' '));

// ========================================
// Horizontal tab escape
// ========================================

static_assert(isNsEscHorizontalTab(U't'));
static_assert(isNsEscHorizontalTab(Tab));

static_assert(!isNsEscHorizontalTab(U'T'));
static_assert(!isNsEscHorizontalTab(U' '));
static_assert(!isNsEscHorizontalTab(U'n'));

// ========================================
// Anchor characters
// ========================================

static_assert(isNsAnchorChar(U'a'));
static_assert(isNsAnchorChar(U'0'));
static_assert(isNsAnchorChar(U'-'));
static_assert(isNsAnchorChar(U'_'));
static_assert(isNsAnchorChar(U'.'));

static_assert(!isNsAnchorChar(Space));
static_assert(!isNsAnchorChar(Tab));
static_assert(!isNsAnchorChar(CollectEntry));
static_assert(!isNsAnchorChar(SequenceStart));
static_assert(!isNsAnchorChar(SequenceEnd));
static_assert(!isNsAnchorChar(MappingStart));
static_assert(!isNsAnchorChar(MappingEnd));

// ========================================
// Plain-safe flow characters
// ========================================

static_assert(isNsPlainSafeIn(U'a'));
static_assert(isNsPlainSafeIn(U'0'));
static_assert(isNsPlainSafeIn(U'-'));
static_assert(isNsPlainSafeIn(U'_'));
static_assert(isNsPlainSafeIn(U'.'));

static_assert(!isNsPlainSafeIn(Space));
static_assert(!isNsPlainSafeIn(Tab));
static_assert(!isNsPlainSafeIn(CollectEntry));
static_assert(!isNsPlainSafeIn(SequenceStart));
static_assert(!isNsPlainSafeIn(SequenceEnd));
static_assert(!isNsPlainSafeIn(MappingStart));
static_assert(!isNsPlainSafeIn(MappingEnd));

// ========================================
// Runtime smoke tests
// ========================================

TEST_CASE("YAML grammar constants match expected syntax", "[job_yaml][grammar]")
{
    REQUIRE(SequenceEntry == '-');
    REQUIRE(MappingValue == ':');
    REQUIRE(Comment == '#');
    REQUIRE(Escape == '\\');

    REQUIRE(DirectivesEnd == "---"sv);
    REQUIRE(DocumentEnd == "..."sv);
}

TEST_CASE("YAML grammar character classes classify common input", "[job_yaml][grammar]")
{
    REQUIRE(isNsDecDigit(U'7'));
    REQUIRE(isNsHexDigit(U'F'));
    REQUIRE(isNsAsciiLetter(U'Q'));
    REQUIRE(isNsWordChar(U'-'));

    REQUIRE(isSWhite(Space));
    REQUIRE(isBChar(LineFeed));
    REQUIRE(isNsChar(U'a'));
}

TEST_CASE("YAML grammar rejects invalid character-class boundaries", "[job_yaml][grammar]")
{
    REQUIRE_FALSE(isNsDecDigit(U'a'));
    REQUIRE_FALSE(isNsHexDigit(U'g'));
    REQUIRE_FALSE(isNsAsciiLetter(U'0'));

    REQUIRE_FALSE(isCPrintable(0xD800));
    REQUIRE_FALSE(isCPrintable(0xDFFF));
    REQUIRE_FALSE(isCPrintable(0x110000));
}

} // namespace job::yaml::tests