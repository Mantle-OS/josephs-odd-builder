#include <catch2/catch_test_macros.hpp>

#include <job_yaml_escape_kernel.h>

#include "test_job_yaml_fixtures.h"

#include <cstdint>
#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Compile-time behavior
// ========================================

constexpr bool constexprEscapeDecodeTest()
{
    const auto newline = YamlEscapeKernel::decode("\\n"sv);

    if (!newline)
        return false;

    if (newline.codepoint != U'\n')
        return false;

    if (newline.consumed != 2)
        return false;

    const auto euro = YamlEscapeKernel::decode("\\u20AC"sv);

    if (!euro)
        return false;

    if (euro.codepoint != U'\u20AC')
        return false;

    if (euro.consumed != 6)
        return false;

    const auto emoji = YamlEscapeKernel::decode("\\U0001F600"sv);

    if (!emoji)
        return false;

    if (emoji.codepoint != U'\U0001F600')
        return false;

    return emoji.consumed == 10;
}

static_assert(constexprEscapeDecodeTest());

static_assert(YamlEscapeKernel::isUnicodeScalar(U'\0'));
static_assert(YamlEscapeKernel::isUnicodeScalar(U'\uD7FF'));
static_assert(!YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0xD800)));
static_assert(!YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0xDFFF)));
static_assert(YamlEscapeKernel::isUnicodeScalar(U'\uE000'));
static_assert(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x10FFFF)));
static_assert(!YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x110000)));

static_assert(YamlEscapeKernel::utf8Size(U'\0') == 1);
static_assert(YamlEscapeKernel::utf8Size(U'\u007F') == 1);
static_assert(YamlEscapeKernel::utf8Size(U'\u0080') == 2);
static_assert(YamlEscapeKernel::utf8Size(U'\u07FF') == 2);
static_assert(YamlEscapeKernel::utf8Size(U'\u0800') == 3);
static_assert(YamlEscapeKernel::utf8Size(U'\uFFFF') == 3);
static_assert(YamlEscapeKernel::utf8Size(U'\U00010000') == 4);
static_assert(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x10FFFF)) == 4);

// ========================================
// Named escapes
// ========================================

TEST_CASE("YamlEscapeKernel decodes named YAML escapes", "[job_yaml][escape_kernel]")
{
    const EscapeCase cases[] = {
                                {"\\0", U'\0', 2, true},
                                {"\\a", U'\a', 2, true},
                                {"\\b", U'\b', 2, true},
                                {"\\t", U'\t', 2, true},
                                {"\\n", U'\n', 2, true},
                                {"\\v", U'\v', 2, true},
                                {"\\f", U'\f', 2, true},
                                {"\\r", U'\r', 2, true},
                                {"\\e", static_cast<char32_t>(0x1B), 2, true},
                                {"\\ ", U' ', 2, true},
                                {"\\\"", U'"', 2, true},
                                {"\\/", U'/', 2, true},
                                {"\\\\", U'\\', 2, true},
                                {"\\N", static_cast<char32_t>(0x85), 2, true},
                                {"\\_", static_cast<char32_t>(0xA0), 2, true},
                                {"\\L", static_cast<char32_t>(0x2028), 2, true},
                                {"\\P", static_cast<char32_t>(0x2029), 2, true},
                                };

    for (const auto &test : cases) {
        const auto result = YamlEscapeKernel::decode(test.text);

        REQUIRE(result.valid == test.valid);
        REQUIRE(static_cast<bool>(result) == test.valid);

        if (test.valid) {
            REQUIRE(result.codepoint == test.codepoint);
            REQUIRE(result.consumed == test.consumed);
        }
    }
}

TEST_CASE("YamlEscapeKernel decodes literal tab escape", "[job_yaml][escape_kernel]")
{
    constexpr char source[] = {'\\', '\t'};

    const auto result = YamlEscapeKernel::decode(std::string_view{source, 2});

    REQUIRE(result);
    REQUIRE(result.codepoint == U'\t');
    REQUIRE(result.consumed == 2);
}

// ========================================
// Hex escapes
// ========================================

TEST_CASE("YamlEscapeKernel decodes x escapes", "[job_yaml][escape_kernel]")
{
    const EscapeCase cases[] = {
                                {"\\x00", U'\0', 4, true},
                                {"\\x01", static_cast<char32_t>(0x01), 4, true},
                                {"\\x20", U' ', 4, true},
                                {"\\x41", U'A', 4, true},
                                {"\\x7F", static_cast<char32_t>(0x7F), 4, true},
                                {"\\x80", static_cast<char32_t>(0x80), 4, true},
                                {"\\xFF", static_cast<char32_t>(0xFF), 4, true},
                                {"\\xff", static_cast<char32_t>(0xFF), 4, true},
                                {"\\xAa", static_cast<char32_t>(0xAA), 4, true},
                                {"\\xaA", static_cast<char32_t>(0xAA), 4, true},
                                };

    for (const auto &test : cases) {
        const auto result = YamlEscapeKernel::decode(test.text);

        REQUIRE(result.valid == test.valid);
        REQUIRE(result.codepoint == test.codepoint);
        REQUIRE(result.consumed == test.consumed);
    }
}

TEST_CASE("YamlEscapeKernel decodes u escapes", "[job_yaml][escape_kernel]")
{
    const EscapeCase cases[] = {
                                {"\\u0000", U'\0', 6, true},
                                {"\\u0041", U'A', 6, true},
                                {"\\u007F", static_cast<char32_t>(0x7F), 6, true},
                                {"\\u0080", static_cast<char32_t>(0x80), 6, true},
                                {"\\u07FF", static_cast<char32_t>(0x07FF), 6, true},
                                {"\\u0800", static_cast<char32_t>(0x0800), 6, true},
                                {"\\u20AC", U'\u20AC', 6, true},
                                {"\\ud7fF", static_cast<char32_t>(0xD7FF), 6, true},
                                {"\\uE000", U'\uE000', 6, true},
                                {"\\uFFFD", U'\uFFFD', 6, true},
                                };

    for (const auto &test : cases) {
        const auto result = YamlEscapeKernel::decode(test.text);

        REQUIRE(result.valid == test.valid);
        REQUIRE(result.codepoint == test.codepoint);
        REQUIRE(result.consumed == test.consumed);
    }
}

TEST_CASE("YamlEscapeKernel decodes U escapes", "[job_yaml][escape_kernel]")
{
    const EscapeCase cases[] = {
                                {"\\U00000000", U'\0', 10, true},
                                {"\\U00000041", U'A', 10, true},
                                {"\\U0000007F", static_cast<char32_t>(0x7F), 10, true},
                                {"\\U00000080", static_cast<char32_t>(0x80), 10, true},
                                {"\\U000007FF", static_cast<char32_t>(0x07FF), 10, true},
                                {"\\U00000800", static_cast<char32_t>(0x0800), 10, true},
                                {"\\U000020AC", U'\u20AC', 10, true},
                                {"\\U00010000", static_cast<char32_t>(0x10000), 10, true},
                                {"\\U0001F600", U'\U0001F600', 10, true},
                                {"\\U0010FFFF", static_cast<char32_t>(0x10FFFF), 10, true},
                                {"\\U0001f600", U'\U0001F600', 10, true},
                                };

    for (const auto &test : cases) {
        const auto result = YamlEscapeKernel::decode(test.text);

        REQUIRE(result.valid == test.valid);
        REQUIRE(result.codepoint == test.codepoint);
        REQUIRE(result.consumed == test.consumed);
    }
}

// ========================================
// Invalid escape syntax
// ========================================

TEST_CASE("YamlEscapeKernel rejects empty and incomplete escapes", "[job_yaml][escape_kernel]")
{
    const std::string_view cases[] = {
        "",
        "\\",
        "\\x",
        "\\x0",
        "\\u",
        "\\u0",
        "\\u00",
        "\\u000",
        "\\U",
        "\\U0",
        "\\U00",
        "\\U000",
        "\\U0000",
        "\\U00000",
        "\\U000000",
        "\\U0000000",
    };

    for (const auto text : cases) {
        const auto result = YamlEscapeKernel::decode(text);

        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.valid);
    }
}

TEST_CASE("YamlEscapeKernel rejects unknown named escapes", "[job_yaml][escape_kernel]")
{
    const std::string_view cases[] = {
        "\\c",
        "\\d",
        "\\g",
        "\\q",
        "\\z",
        "\\?",
        "\\@",
    };

    for (const auto text : cases) {
        const auto result = YamlEscapeKernel::decode(text);

        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.valid);
    }
}

TEST_CASE("YamlEscapeKernel rejects non hexadecimal digits", "[job_yaml][escape_kernel]")
{
    const std::string_view cases[] = {
        "\\xGG",
        "\\x0G",
        "\\xG0",
        "\\uGGGG",
        "\\u000G",
        "\\uG000",
        "\\U0000000G",
        "\\U000000G0",
        "\\UG0000000",
    };

    for (const auto text : cases) {
        const auto result = YamlEscapeKernel::decode(text);

        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.valid);
    }
}

// ========================================
// Unicode scalar validity
// ========================================

TEST_CASE("YamlEscapeKernel identifies Unicode scalar values", "[job_yaml][escape_kernel]")
{
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x000000)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00007F)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x000080)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00D7FF)));

    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00D800)));
    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00DBFF)));
    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00DC00)));
    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00DFFF)));

    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00E000)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x00FFFF)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x010000)));
    REQUIRE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x10FFFF)));

    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0x110000)));
    REQUIRE_FALSE(YamlEscapeKernel::isUnicodeScalar(static_cast<char32_t>(0xFFFFFFFF)));
}

TEST_CASE("YamlEscapeKernel rejects UTF-16 surrogate escapes", "[job_yaml][escape_kernel]")
{
    const std::string_view cases[] = {
        "\\uD800",
        "\\uDBFF",
        "\\uDC00",
        "\\uDFFF",
        "\\U0000D800",
        "\\U0000DBFF",
        "\\U0000DC00",
        "\\U0000DFFF",
    };

    for (const auto text : cases) {
        const auto result = YamlEscapeKernel::decode(text);

        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.valid);
    }
}

TEST_CASE("YamlEscapeKernel accepts Unicode values adjacent to surrogate range", "[job_yaml][escape_kernel]")
{
    const auto before = YamlEscapeKernel::decode("\\uD7FF");
    const auto after = YamlEscapeKernel::decode("\\uE000");

    REQUIRE(before);
    REQUIRE(before.codepoint == static_cast<char32_t>(0xD7FF));

    REQUIRE(after);
    REQUIRE(after.codepoint == static_cast<char32_t>(0xE000));
}

TEST_CASE("YamlEscapeKernel rejects codepoints beyond Unicode maximum", "[job_yaml][escape_kernel]")
{
    const std::string_view cases[] = {
        "\\U00110000",
        "\\U001FFFFF",
        "\\U01000000",
        "\\UFFFFFFFF",
    };

    for (const auto text : cases) {
        const auto result = YamlEscapeKernel::decode(text);

        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.valid);
    }
}

// ========================================
// Consumed byte count
// ========================================

TEST_CASE("YamlEscapeKernel reports exact consumed source size", "[job_yaml][escape_kernel]")
{
    const auto named = YamlEscapeKernel::decode("\\ntrailing");
    const auto hex8 = YamlEscapeKernel::decode("\\x41trailing");
    const auto hex16 = YamlEscapeKernel::decode("\\u20ACtrailing");
    const auto hex32 = YamlEscapeKernel::decode("\\U0001F600trailing");

    REQUIRE(named);
    REQUIRE(named.codepoint == U'\n');
    REQUIRE(named.consumed == 2);

    REQUIRE(hex8);
    REQUIRE(hex8.codepoint == U'A');
    REQUIRE(hex8.consumed == 4);

    REQUIRE(hex16);
    REQUIRE(hex16.codepoint == U'\u20AC');
    REQUIRE(hex16.consumed == 6);

    REQUIRE(hex32);
    REQUIRE(hex32.codepoint == U'\U0001F600');
    REQUIRE(hex32.consumed == 10);
}

// ========================================
// UTF-8 size
// ========================================

TEST_CASE("YamlEscapeKernel reports UTF-8 encoded size boundaries", "[job_yaml][escape_kernel]")
{
    SECTION("one byte")
    {
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0000)) == 1);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0001)) == 1);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x007F)) == 1);
    }

    SECTION("two bytes")
    {
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0080)) == 2);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0081)) == 2);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x07FF)) == 2);
    }

    SECTION("three bytes")
    {
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0800)) == 3);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x0801)) == 3);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0xD7FF)) == 3);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0xE000)) == 3);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0xFFFF)) == 3);
    }

    SECTION("four bytes")
    {
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x10000)) == 4);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x10001)) == 4);
        REQUIRE(YamlEscapeKernel::utf8Size(static_cast<char32_t>(0x10FFFF)) == 4);
    }
}

// ========================================
// UTF-8 encoding
// ========================================

TEST_CASE("YamlEscapeKernel encodes one byte UTF-8", "[job_yaml][escape_kernel]")
{
    char output[4]{};

    REQUIRE(YamlEscapeKernel::encodeUtf8(U'A', output) == 1);
    REQUIRE(static_cast<std::uint8_t>(output[0]) == 0x41);
}

TEST_CASE("YamlEscapeKernel encodes two byte UTF-8 boundaries", "[job_yaml][escape_kernel]")
{
    SECTION("U+0080")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0x0080), output) == 2);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xC2);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x80);
    }

    SECTION("U+07FF")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0x07FF), output) == 2);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xDF);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0xBF);
    }
}

TEST_CASE("YamlEscapeKernel encodes three byte UTF-8 boundaries", "[job_yaml][escape_kernel]")
{
    SECTION("U+0800")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0x0800), output) == 3);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xE0);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0xA0);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0x80);
    }

    SECTION("Euro sign")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(U'\u20AC', output) == 3);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xE2);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x82);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0xAC);
    }

    SECTION("U+FFFF")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0xFFFF), output) == 3);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xEF);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0xBF);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0xBF);
    }
}

TEST_CASE("YamlEscapeKernel encodes four byte UTF-8 boundaries", "[job_yaml][escape_kernel]")
{
    SECTION("U+10000")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0x10000), output) == 4);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xF0);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x90);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0x80);
        REQUIRE(static_cast<std::uint8_t>(output[3]) == 0x80);
    }

    SECTION("U+1F600")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(U'\U0001F600', output) == 4);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xF0);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x9F);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0x98);
        REQUIRE(static_cast<std::uint8_t>(output[3]) == 0x80);
    }

    SECTION("U+10FFFF")
    {
        char output[4]{};

        REQUIRE(YamlEscapeKernel::encodeUtf8(static_cast<char32_t>(0x10FFFF), output) == 4);
        REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xF4);
        REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x8F);
        REQUIRE(static_cast<std::uint8_t>(output[2]) == 0xBF);
        REQUIRE(static_cast<std::uint8_t>(output[3]) == 0xBF);
    }
}

// ========================================
// Decode -> UTF-8 pipeline
// ========================================

TEST_CASE("YamlEscapeKernel decoded codepoint can be emitted directly as UTF-8",
          "[job_yaml][escape_kernel]")
{
    const auto result = YamlEscapeKernel::decode("\\u20AC");

    REQUIRE(result);
    REQUIRE(result.codepoint == U'\u20AC');

    char output[4]{};
    const auto size = YamlEscapeKernel::encodeUtf8(result.codepoint, output);

    REQUIRE(size == 3);
    REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xE2);
    REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x82);
    REQUIRE(static_cast<std::uint8_t>(output[2]) == 0xAC);
}

TEST_CASE("YamlEscapeKernel four byte decoded codepoint can be emitted directly as UTF-8",
          "[job_yaml][escape_kernel]")
{
    const auto result = YamlEscapeKernel::decode("\\U0001F600");

    REQUIRE(result);
    REQUIRE(result.codepoint == U'\U0001F600');

    char output[4]{};
    const auto size = YamlEscapeKernel::encodeUtf8(result.codepoint, output);

    REQUIRE(size == 4);
    REQUIRE(static_cast<std::uint8_t>(output[0]) == 0xF0);
    REQUIRE(static_cast<std::uint8_t>(output[1]) == 0x9F);
    REQUIRE(static_cast<std::uint8_t>(output[2]) == 0x98);
    REQUIRE(static_cast<std::uint8_t>(output[3]) == 0x80);
}

} // namespace job::yaml::tests