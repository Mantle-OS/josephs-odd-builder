#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "unicode_normalizer.h"

using job::token::UnicodeNormalizer;

//
// Block 1: usage / examples
//

TEST_CASE("UnicodeNormalizer validates ordinary UTF-8 text", "[token][unicode][usage]")
{
    REQUIRE(UnicodeNormalizer::isValidUtf8("hello world"));
    REQUIRE(UnicodeNormalizer::isValidUtf8("caf\xC3\xA9"));
    REQUIRE(UnicodeNormalizer::isValidUtf8("\xE6\x97\xA5\xE6\x9C\xAC"));
    REQUIRE(UnicodeNormalizer::isValidUtf8("\xF0\x9F\x90\x95"));
}

TEST_CASE("UnicodeNormalizer nextCodepoint walks mixed UTF-8 text", "[token][unicode][usage]")
{
    const std::string text = "A" "\xC3\xA9" "\xE6\x97\xA5" "\xF0\x9F\x90\x95";

    std::size_t offset{0};
    std::uint32_t codepoint{0};

    REQUIRE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
    REQUIRE(codepoint == 0x41);
    REQUIRE(offset == 1);

    REQUIRE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
    REQUIRE(codepoint == 0x00E9);
    REQUIRE(offset == 3);

    REQUIRE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
    REQUIRE(codepoint == 0x65E5);
    REQUIRE(offset == 6);

    REQUIRE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
    REQUIRE(codepoint == 0x1F415);
    REQUIRE(offset == text.size());

    REQUIRE_FALSE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
}

TEST_CASE("UnicodeNormalizer encodeCodepoint produces UTF-8", "[token][unicode][usage]")
{
    std::array<char, 4> buffer{};

    std::size_t count = UnicodeNormalizer::encodeCodepoint(0x41, buffer.data());
    REQUIRE(count == 1);
    REQUIRE(std::string_view{buffer.data(), count} == "A");

    count = UnicodeNormalizer::encodeCodepoint(0x00E9, buffer.data());
    REQUIRE(count == 2);
    REQUIRE(std::string_view{buffer.data(), count} == "\xC3\xA9");

    count = UnicodeNormalizer::encodeCodepoint(0x65E5, buffer.data());
    REQUIRE(count == 3);
    REQUIRE(std::string_view{buffer.data(), count} == "\xE6\x97\xA5");

    count = UnicodeNormalizer::encodeCodepoint(0x1F415, buffer.data());
    REQUIRE(count == 4);
    REQUIRE(std::string_view{buffer.data(), count} == "\xF0\x9F\x90\x95");
}

TEST_CASE("UnicodeNormalizer codepoint encode and decode round trip", "[token][unicode][usage]")
{
    constexpr std::array<std::uint32_t, 8> codepoints = {0x00, 0x41, 0x7F, 0x80, 0x7FF, 0x800, 0x65E5, 0x10FFFF};

    for (const std::uint32_t expected : codepoints) {
        std::array<char, 4> buffer{};
        const std::size_t length = UnicodeNormalizer::encodeCodepoint(expected, buffer.data());

        REQUIRE(length > 0);

        const std::string_view text{buffer.data(), length};
        std::size_t offset{0};
        std::uint32_t decoded{0};

        REQUIRE(UnicodeNormalizer::nextCodepoint(text, offset, decoded));
        REQUIRE(decoded == expected);
        REQUIRE(offset == length);
    }
}

TEST_CASE("UnicodeNormalizer escapes spaces with SentencePiece marker", "[token][unicode][sentencepiece][usage]")
{
    const std::string escaped = UnicodeNormalizer::escapeSpaces("hello world", false);
    REQUIRE(escaped == "hello\xE2\x96\x81world");
}

TEST_CASE("UnicodeNormalizer can prepend SentencePiece dummy space", "[token][unicode][sentencepiece][usage]")
{
    const std::string escaped = UnicodeNormalizer::escapeSpaces("hello world");
    REQUIRE(escaped == "\xE2\x96\x81hello\xE2\x96\x81world");
}

TEST_CASE("UnicodeNormalizer does not duplicate existing leading SentencePiece marker", "[token][unicode][sentencepiece][usage]")
{
    const std::string text = "\xE2\x96\x81hello";
    REQUIRE(UnicodeNormalizer::escapeSpaces(text, true) == text);
}

TEST_CASE("UnicodeNormalizer unescapes SentencePiece spaces", "[token][unicode][sentencepiece][usage]")
{
    const std::string text = "hello\xE2\x96\x81world";
    REQUIRE(UnicodeNormalizer::unescapeSpaces(text) == "hello world");
}

TEST_CASE("UnicodeNormalizer can strip leading SentencePiece dummy space", "[token][unicode][sentencepiece][usage]")
{
    const std::string text = "\xE2\x96\x81hello\xE2\x96\x81world";
    REQUIRE(UnicodeNormalizer::unescapeSpaces(text, true) == "hello world");
    REQUIRE(UnicodeNormalizer::unescapeSpaces(text, false) == " hello world");
}

TEST_CASE("UnicodeNormalizer SentencePiece escape and unescape round trip", "[token][unicode][sentencepiece][usage]")
{
    const std::string original = "the quick brown fox";
    const std::string escaped = UnicodeNormalizer::escapeSpaces(original, true);
    const std::string restored = UnicodeNormalizer::unescapeSpaces(escaped, true);

    REQUIRE(restored == original);
}

TEST_CASE("UnicodeNormalizer trims leading and trailing ASCII whitespace", "[token][unicode][usage]")
{
    REQUIRE(UnicodeNormalizer::trimWhitespace(" \t\r\nhello world \n\t") == "hello world");
    REQUIRE(UnicodeNormalizer::trimWhitespace("already-clean") == "already-clean");
}

TEST_CASE("UnicodeNormalizer default normalize preserves input", "[token][unicode][normalize][usage]")
{
    UnicodeNormalizer::Options options;
    REQUIRE(UnicodeNormalizer::normalize("hello   world", options) == "hello   world");
}

TEST_CASE("UnicodeNormalizer can collapse runs of whitespace", "[token][unicode][normalize][usage]")
{
    UnicodeNormalizer::Options options;
    options.cleanExtraSpaces = true;

    REQUIRE(UnicodeNormalizer::normalize("hello \t \n world", options) == "hello world");
}

TEST_CASE("UnicodeNormalizer can add ordinary dummy prefix space", "[token][unicode][normalize][usage]")
{
    UnicodeNormalizer::Options options;
    options.addDummyPrefixSpace = true;

    REQUIRE(UnicodeNormalizer::normalize("hello", options) == " hello");
    REQUIRE(UnicodeNormalizer::normalize(" hello", options) == " hello");
}

TEST_CASE("UnicodeNormalizer can normalize into SentencePiece space markers", "[token][unicode][normalize][usage]")
{
    UnicodeNormalizer::Options options;
    options.addDummyPrefixSpace = true;
    options.replaceSpacesWithMarker = true;

    REQUIRE(UnicodeNormalizer::normalize("hello world", options) == "\xE2\x96\x81hello\xE2\x96\x81world");
}

TEST_CASE("UnicodeNormalizer combines whitespace cleanup with SentencePiece escaping", "[token][unicode][normalize][usage]")
{
    UnicodeNormalizer::Options options;
    options.addDummyPrefixSpace = true;
    options.replaceSpacesWithMarker = true;
    options.cleanExtraSpaces = true;

    REQUIRE(UnicodeNormalizer::normalize("hello \t \n world", options) == "\xE2\x96\x81hello\xE2\x96\x81world");
}

//
// Block 2: edge cases / invalid UTF-8
//

TEST_CASE("UnicodeNormalizer accepts empty UTF-8 input", "[token][unicode][edge]")
{
    REQUIRE(UnicodeNormalizer::isValidUtf8(""));
}

TEST_CASE("UnicodeNormalizer rejects stray continuation bytes", "[token][unicode][edge][invalid]")
{
    const std::string text{static_cast<char>(0x80)};
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(text));
}

TEST_CASE("UnicodeNormalizer rejects truncated UTF-8 sequences", "[token][unicode][edge][invalid]")
{
    const std::string twoByte{static_cast<char>(0xC2)};
    const std::string threeByte{static_cast<char>(0xE2), static_cast<char>(0x82)};
    const std::string fourByte{static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x90)};

    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(twoByte));
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(threeByte));
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(fourByte));
}

TEST_CASE("UnicodeNormalizer rejects overlong UTF-8 encodings", "[token][unicode][edge][invalid]")
{
    const std::string twoByteOverlong{static_cast<char>(0xC0), static_cast<char>(0x80)};
    const std::string threeByteOverlong{static_cast<char>(0xE0), static_cast<char>(0x80), static_cast<char>(0x80)};
    const std::string fourByteOverlong{static_cast<char>(0xF0), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80)};

    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(twoByteOverlong));
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(threeByteOverlong));
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(fourByteOverlong));
}

TEST_CASE("UnicodeNormalizer rejects UTF-16 surrogate code points", "[token][unicode][edge][invalid]")
{
    const std::string surrogate{static_cast<char>(0xED), static_cast<char>(0xA0), static_cast<char>(0x80)};
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(surrogate));

    std::array<char, 4> buffer{};

    REQUIRE(UnicodeNormalizer::encodeCodepoint(0xD800, buffer.data()) == 0);
    REQUIRE(UnicodeNormalizer::encodeCodepoint(0xDFFF, buffer.data()) == 0);
}

TEST_CASE("UnicodeNormalizer rejects code points above Unicode maximum", "[token][unicode][edge][invalid]")
{
    const std::string aboveMaximum{static_cast<char>(0xF4), static_cast<char>(0x90), static_cast<char>(0x80), static_cast<char>(0x80)};
    REQUIRE_FALSE(UnicodeNormalizer::isValidUtf8(aboveMaximum));

    std::array<char, 4> buffer{};
    REQUIRE(UnicodeNormalizer::encodeCodepoint(0x110000, buffer.data()) == 0);
}

TEST_CASE("UnicodeNormalizer encodeCodepoint rejects null output buffer", "[token][unicode][edge]")
{
    REQUIRE(UnicodeNormalizer::encodeCodepoint(0x41, nullptr) == 0);
}

TEST_CASE("UnicodeNormalizer nextCodepoint rejects end offset", "[token][unicode][edge]")
{
    const std::string text = "hello";
    std::size_t offset = text.size();
    std::uint32_t codepoint{0};

    REQUIRE_FALSE(UnicodeNormalizer::nextCodepoint(text, offset, codepoint));
    REQUIRE(offset == text.size());
}

TEST_CASE("UnicodeNormalizer trimWhitespace handles all-whitespace and empty input", "[token][unicode][edge]")
{
    REQUIRE(UnicodeNormalizer::trimWhitespace("").empty());
    REQUIRE(UnicodeNormalizer::trimWhitespace(" \t\r\n ").empty());
}

TEST_CASE("UnicodeNormalizer escapeSpaces handles empty input", "[token][unicode][sentencepiece][edge]")
{
    REQUIRE(UnicodeNormalizer::escapeSpaces("", false).empty());
    REQUIRE(UnicodeNormalizer::escapeSpaces("", true) == UnicodeNormalizer::kSentencePieceSpace);
}

TEST_CASE("UnicodeNormalizer unescapeSpaces handles empty input", "[token][unicode][sentencepiece][edge]")
{
    REQUIRE(UnicodeNormalizer::unescapeSpaces("").empty());
}

TEST_CASE("UnicodeNormalizer normalize preserves current empty dummy-prefix behavior", "[token][unicode][normalize][edge]")
{
    UnicodeNormalizer::Options options;

    REQUIRE(UnicodeNormalizer::normalize("", options).empty());

    options.addDummyPrefixSpace = true;

    REQUIRE(UnicodeNormalizer::normalize("", options) == UnicodeNormalizer::kSentencePieceSpace);
}