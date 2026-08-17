#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "byte_fallback.h"

using job::token::ByteFallback;

//
// Block 1: usage / examples
//

TEST_CASE("ByteFallback recognizes canonical byte token strings", "[token][byte-fallback][usage]")
{
    REQUIRE(ByteFallback::isByteToken("<0x00>"));
    REQUIRE(ByteFallback::isByteToken("<0x41>"));
    REQUIRE(ByteFallback::isByteToken("<0x7F>"));
    REQUIRE(ByteFallback::isByteToken("<0xFF>"));
    REQUIRE_FALSE(ByteFallback::isByteToken("A"));
    REQUIRE_FALSE(ByteFallback::isByteToken("<0x0>"));
    REQUIRE_FALSE(ByteFallback::isByteToken("<0x000>"));
    REQUIRE_FALSE(ByteFallback::isByteToken("<0xGG>"));
}

TEST_CASE("ByteFallback parses canonical byte token strings", "[token][byte-fallback][usage]")
{
    std::uint8_t value{0};

    REQUIRE(ByteFallback::parseByteToken("<0x00>", value));
    REQUIRE(value == 0x00);
    REQUIRE(ByteFallback::parseByteToken("<0x41>", value));
    REQUIRE(value == 0x41);
    REQUIRE(ByteFallback::parseByteToken("<0x7F>", value));
    REQUIRE(value == 0x7F);
    REQUIRE(ByteFallback::parseByteToken("<0xFF>", value));
    REQUIRE(value == 0xFF);
}

TEST_CASE("ByteFallback accepts lowercase hexadecimal input", "[token][byte-fallback][usage]")
{
    std::uint8_t value{0};

    REQUIRE(ByteFallback::parseByteToken("<0x0a>", value));
    REQUIRE(value == 0x0A);
    REQUIRE(ByteFallback::parseByteToken("<0xff>", value));
    REQUIRE(value == 0xFF);
    REQUIRE(ByteFallback::parseByteToken("<0xAf>", value));
    REQUIRE(value == 0xAF);
}

TEST_CASE("ByteFallback formats bytes using canonical uppercase hexadecimal", "[token][byte-fallback][usage]")
{
    REQUIRE(ByteFallback::formatByte(0x00) == "<0x00>");
    REQUIRE(ByteFallback::formatByte(0x09) == "<0x09>");
    REQUIRE(ByteFallback::formatByte(0x0A) == "<0x0A>");
    REQUIRE(ByteFallback::formatByte(0x41) == "<0x41>");
    REQUIRE(ByteFallback::formatByte(0xAF) == "<0xAF>");
    REQUIRE(ByteFallback::formatByte(0xFF) == "<0xFF>");
}

TEST_CASE("ByteFallback byteToStringView returns canonical static representation", "[token][byte-fallback][usage]")
{
    const std::string_view zero = ByteFallback::byteToStringView(0x00);
    const std::string_view letter = ByteFallback::byteToStringView(0x41);
    const std::string_view maximum = ByteFallback::byteToStringView(0xFF);

    REQUIRE(zero == "<0x00>");
    REQUIRE(letter == "<0x41>");
    REQUIRE(maximum == "<0xFF>");
}

TEST_CASE("ByteFallback format and parse round trip bytes", "[token][byte-fallback][usage]")
{
    constexpr std::array<std::uint8_t, 8> values = {0x00, 0x01, 0x09, 0x0A, 0x41, 0x7F, 0x80, 0xFF};

    for (const std::uint8_t expected : values) {
        const std::string token = ByteFallback::formatByte(expected);
        std::uint8_t parsed{0};

        REQUIRE(ByteFallback::parseByteToken(token, parsed));
        REQUIRE(parsed == expected);
    }
}

TEST_CASE("ByteFallback static string table covers every byte value", "[token][byte-fallback][usage]")
{
    for (std::uint32_t i = 0; i <= 0xFF; ++i) {
        const auto byte = static_cast<std::uint8_t>(i);
        const std::string_view token = ByteFallback::byteToStringView(byte);

        REQUIRE(token.size() == 6);
        REQUIRE(token.starts_with("<0x"));
        REQUIRE(token.ends_with(">"));

        std::uint8_t parsed{0};

        REQUIRE(ByteFallback::parseByteToken(token, parsed));
        REQUIRE(parsed == byte);
    }
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("ByteFallback rejects malformed byte token strings", "[token][byte-fallback][edge]")
{
    std::uint8_t value{0xAA};

    REQUIRE_FALSE(ByteFallback::parseByteToken("", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0x>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0x0>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0x000>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("0x41", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("[0x41]", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0X41>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0x4G>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0xG4>", value));
    REQUIRE_FALSE(ByteFallback::parseByteToken("<0x41>extra", value));
}

TEST_CASE("ByteFallback failed parse does not produce a valid result", "[token][byte-fallback][edge]")
{
    std::uint8_t value{0x5A};

    REQUIRE_FALSE(ByteFallback::parseByteToken("<0xZZ>", value));

    // The parser contract does not require output mutation on failure.
    // We only care that malformed input is rejected.
}

TEST_CASE("ByteFallback canonical representations are unique", "[token][byte-fallback][edge]")
{
    for (std::uint32_t left = 0; left <= 0xFF; ++left) {
        const std::string_view leftToken = ByteFallback::byteToStringView(static_cast<std::uint8_t>(left));

        for (std::uint32_t right = left + 1; right <= 0xFF; ++right) {
            const std::string_view rightToken = ByteFallback::byteToStringView(static_cast<std::uint8_t>(right));
            REQUIRE(leftToken != rightToken);
        }
    }
}