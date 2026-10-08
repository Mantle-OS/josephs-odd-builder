#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_string_utils.h>

using namespace job::core;

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("JobStringUtils exposes string and integer concepts", "[core][string-utils][usage][concept]")
{
    STATIC_CHECK(JobStringType<std::string>);
    STATIC_CHECK(JobStringType<std::string_view>);

    STATIC_CHECK_FALSE(JobStringType<const char *>);
    STATIC_CHECK_FALSE(JobStringType<int>);

    STATIC_CHECK(JobIntegerType<std::uint8_t>);
    STATIC_CHECK(JobIntegerType<std::uint16_t>);
    STATIC_CHECK(JobIntegerType<std::uint32_t>);
    STATIC_CHECK(JobIntegerType<std::uint64_t>);
    STATIC_CHECK(JobIntegerType<std::int32_t>);

    STATIC_CHECK_FALSE(JobIntegerType<bool>);
    STATIC_CHECK_FALSE(JobIntegerType<float>);
}

TEST_CASE("JobStringUtils converts strings to integers", "[core][string-utils][usage][integer]")
{
    CHECK(JobStringUtils::toUInt64(std::string_view{"12345"}) == 12345);
    CHECK(JobStringUtils::toUInt64(std::string_view{"FF"}, 16) == 255);

    CHECK(JobStringUtils::toInteger<std::uint16_t>(std::string_view{"65535"}) == 65535);
    CHECK(JobStringUtils::toInteger<std::int32_t>(std::string_view{"-42"}) == -42);
    CHECK(JobStringUtils::toInteger<std::uint8_t>(std::string_view{"7F"}, 16) == 0x7F);
}

TEST_CASE("JobStringUtils converts string case", "[core][string-utils][usage][case]")
{
    CHECK(JobStringUtils::toLower("Hello WORLD") == "hello world");
    CHECK(JobStringUtils::toUpper("Hello world") == "HELLO WORLD");
}

TEST_CASE("JobStringUtils exposes string data and size", "[core][string-utils][usage][view]")
{
    const std::string value{"hello"};

    CHECK(JobStringUtils::data(value) == value.data());
    CHECK(JobStringUtils::size(value) == 5);

    constexpr std::string_view view{"world"};

    STATIC_CHECK(JobStringUtils::data(view) == view.data());
    STATIC_CHECK(JobStringUtils::size(view) == 5);
}

TEST_CASE("JobStringUtils converts strings to and from hex", "[core][string-utils][usage][hex]")
{
    CHECK(JobStringUtils::toHex(std::string_view{"Hello"}) == "0x48656C6C6F");
    CHECK(JobStringUtils::toHex(std::string_view{"Hello"}, ':') == "48:65:6C:6C:6F");

    CHECK(JobStringUtils::fromHex(std::string_view{"0x48656C6C6F"}) == "Hello");
    CHECK(JobStringUtils::fromHex(std::string_view{"48:65:6C:6C:6F"}, ':') == "Hello");
}

TEST_CASE("JobStringUtils replaces string content", "[core][string-utils][usage][replace]")
{
    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              std::string_view{"this"},
              std::string_view{"that"}) == "that is");

    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              std::string_view{"is cool"},
              2) == "this is cool");

    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              's',
              std::string_view{"s cool"},
              2) == "this is cool");
}

TEST_CASE("JobStringUtils splits strings", "[core][string-utils][usage][split]")
{
    CHECK(JobStringUtils::split(std::string_view{"this is"}, 2) == "is is");

    const auto parts = JobStringUtils::split(std::string_view{"this is"});

    REQUIRE(parts.size() == 2);
    CHECK(parts[0] == "this");
    CHECK(parts[1] == "is");
}

TEST_CASE("JobStringUtils removes leading and trailing string noise", "[core][string-utils][usage][clean]")
{
    CHECK(JobStringUtils::noHead(std::string_view{"  this is"}) == "this is");
    CHECK(JobStringUtils::noTail(std::string_view{"this is\r           "}) == "this is");
}

TEST_CASE("JobStringUtils encodes Unicode scalars", "[core][string-utils][usage][unicode]")
{
    const auto ascii = text::UnicodeScalar::from(U'A');
    const auto euro = text::UnicodeScalar::from(0x20AC);
    const auto rocket = text::UnicodeScalar::from(0x1F680);

    REQUIRE(ascii.has_value());
    REQUIRE(euro.has_value());
    REQUIRE(rocket.has_value());

    const std::array scalars{*ascii, *euro, *rocket};

    const auto utf8 = JobStringUtils::toUtf8(scalars);

    REQUIRE(utf8.size() == 8);
    CHECK(static_cast<std::uint8_t>(utf8[0]) == 0x41);
    CHECK(static_cast<std::uint8_t>(utf8[1]) == 0xE2);
    CHECK(static_cast<std::uint8_t>(utf8[2]) == 0x82);
    CHECK(static_cast<std::uint8_t>(utf8[3]) == 0xAC);
    CHECK(static_cast<std::uint8_t>(utf8[4]) == 0xF0);
    CHECK(static_cast<std::uint8_t>(utf8[5]) == 0x9F);
    CHECK(static_cast<std::uint8_t>(utf8[6]) == 0x9A);
    CHECK(static_cast<std::uint8_t>(utf8[7]) == 0x80);

    const auto utf16 = JobStringUtils::toUtf16(scalars);

    REQUIRE(utf16.size() == 4);
    CHECK(utf16[0] == u'A');
    CHECK(utf16[1] == 0x20AC);
    CHECK(utf16[2] == 0xD83D);
    CHECK(utf16[3] == 0xDE80);

    const auto utf32 = JobStringUtils::toUtf32(scalars);

    REQUIRE(utf32.size() == 3);
    CHECK(utf32[0] == U'A');
    CHECK(utf32[1] == 0x20AC);
    CHECK(utf32[2] == 0x1F680);
}

TEST_CASE("JobStringUtils decodes Unicode text", "[core][string-utils][usage][unicode]")
{
    constexpr std::array<std::uint8_t, 8> utf8{
        0x41,
        0xE2, 0x82, 0xAC,
        0xF0, 0x9F, 0x9A, 0x80
    };

    const auto utf8Scalars = JobStringUtils::fromUtf8(utf8);

    REQUIRE(utf8Scalars.size() == 3);
    CHECK(utf8Scalars[0].value() == U'A');
    CHECK(utf8Scalars[1].value() == 0x20AC);
    CHECK(utf8Scalars[2].value() == 0x1F680);

    constexpr std::array<char16_t, 4> utf16{
        u'A',
        0x20AC,
        0xD83D, 0xDE80
    };

    const auto utf16Scalars = JobStringUtils::fromUtf16(utf16);

    REQUIRE(utf16Scalars.size() == 3);
    CHECK(utf16Scalars[0].value() == U'A');
    CHECK(utf16Scalars[1].value() == 0x20AC);
    CHECK(utf16Scalars[2].value() == 0x1F680);

    constexpr std::array<char32_t, 3> utf32{
        U'A',
        0x20AC,
        0x1F680
    };

    const auto utf32Scalars = JobStringUtils::fromUtf32(utf32);

    REQUIRE(utf32Scalars.size() == 3);
    CHECK(utf32Scalars[0].value() == U'A');
    CHECK(utf32Scalars[1].value() == 0x20AC);
    CHECK(utf32Scalars[2].value() == 0x1F680);
}

TEST_CASE("JobStringUtils converts ASCII directly", "[core][string-utils][usage][ascii]")
{
    const auto h = text::UnicodeScalar::from(U'H');
    const auto e = text::UnicodeScalar::from(U'e');
    const auto l = text::UnicodeScalar::from(U'l');
    const auto o = text::UnicodeScalar::from(U'o');

    REQUIRE(h.has_value());
    REQUIRE(e.has_value());
    REQUIRE(l.has_value());
    REQUIRE(o.has_value());

    const std::array scalars{*h, *e, *l, *l, *o};

    CHECK(JobStringUtils::toAscii(scalars) == "Hello");

    constexpr std::array<std::uint8_t, 5> ascii{
        'H', 'e', 'l', 'l', 'o'
    };

    const auto decoded = JobStringUtils::fromAscii(ascii);

    REQUIRE(decoded.size() == 5);
    CHECK(decoded[0].value() == U'H');
    CHECK(decoded[1].value() == U'e');
    CHECK(decoded[2].value() == U'l');
    CHECK(decoded[3].value() == U'l');
    CHECK(decoded[4].value() == U'o');
}

// -----------------------------------------------------------------------------
// Block two: edge cases / breakage
// -----------------------------------------------------------------------------

TEST_CASE("JobStringUtils rejects malformed integer strings", "[core][string-utils][edge][integer]")
{
    CHECK(JobStringUtils::toUInt64(std::string_view{"abc"}) == 0);
    CHECK(JobStringUtils::toUInt64(std::string_view{"12x"}) == 0);
    CHECK(JobStringUtils::toUInt64(std::string_view{}) == 0);

    CHECK(JobStringUtils::toInteger<std::uint16_t>(std::string_view{"100000"}) == 0);
    CHECK(JobStringUtils::toInteger<std::int32_t>(std::string_view{"abc"}) == 0);
}

TEST_CASE("JobStringUtils handles empty case conversion", "[core][string-utils][edge][case]")
{
    CHECK(JobStringUtils::toLower("").empty());
    CHECK(JobStringUtils::toUpper("").empty());
}

TEST_CASE("JobStringUtils handles empty string data", "[core][string-utils][edge][view]")
{
    const std::string value;

    CHECK(JobStringUtils::size(value) == 0);
    CHECK(JobStringUtils::data(value) == value.data());
}

TEST_CASE("JobStringUtils handles empty hex conversion", "[core][string-utils][edge][hex]")
{
    CHECK(JobStringUtils::toHex(std::string_view{}) == "0x");
    CHECK(JobStringUtils::toHex(std::string_view{}, ':').empty());

    CHECK(JobStringUtils::fromHex(std::string_view{"0x"}).empty());
    CHECK(JobStringUtils::fromHex(std::string_view{}).empty());
}

TEST_CASE("JobStringUtils rejects malformed hex strings", "[core][string-utils][edge][hex]")
{
    CHECK(JobStringUtils::fromHex(std::string_view{"0x4"}).empty());
    CHECK(JobStringUtils::fromHex(std::string_view{"0xGG"}).empty());

    CHECK(JobStringUtils::fromHex(std::string_view{"48-65"}, ':').empty());
    CHECK(JobStringUtils::fromHex(std::string_view{"4:65"}, ':').empty());
    CHECK(JobStringUtils::fromHex(std::string_view{"48::65"}, ':').empty());
}

TEST_CASE("JobStringUtils rejects trailing hex separators", "[core][string-utils][edge][hex]")
{
    CHECK(JobStringUtils::fromHex(std::string_view{"48:65:"}, ':').empty());
}

TEST_CASE("JobStringUtils replace preserves input when text is not found", "[core][string-utils][edge][replace]")
{
    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              std::string_view{"missing"},
              std::string_view{"that"}) == "this is");

    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              'x',
              std::string_view{"replacement"},
              1) == "this is");
}

TEST_CASE("JobStringUtils positional replace handles boundaries", "[core][string-utils][edge][replace]")
{
    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              std::string_view{"replacement"},
              100) == "this is");

    CHECK(JobStringUtils::replace(
              std::string_view{"this is"},
              std::string_view{"everything"},
              7) == "everything");
}

TEST_CASE("JobStringUtils occurrence replace handles repeated characters", "[core][string-utils][edge][replace]")
{
    CHECK(JobStringUtils::replace(
              std::string_view{"ssss"},
              's',
              std::string_view{"X"},
              1) == "Xsss");

    CHECK(JobStringUtils::replace(
              std::string_view{"ssss"},
              's',
              std::string_view{"X"},
              4) == "sssX");

    CHECK(JobStringUtils::replace(
              std::string_view{"ssss"},
              's',
              std::string_view{"X"},
              5) == "ssss");
}

TEST_CASE("JobStringUtils positional split handles boundaries", "[core][string-utils][edge][split]")
{
    CHECK(JobStringUtils::split(std::string_view{"this is"}, 1) == "his is");
    CHECK(JobStringUtils::split(std::string_view{"this is"}, 6) == "s");
    CHECK(JobStringUtils::split(std::string_view{"this is"}, 7).empty());
    CHECK(JobStringUtils::split(std::string_view{"this is"}, 100).empty());
}

TEST_CASE("JobStringUtils separator split handles repeated and missing separators", "[core][string-utils][edge][split]")
{
    {
        const auto parts = JobStringUtils::split(std::string_view{"this"});

        REQUIRE(parts.size() == 1);
        CHECK(parts[0] == "this");
    }

    {
        const auto parts = JobStringUtils::split(std::string_view{"this  is"});

        REQUIRE(parts.size() == 3);
        CHECK(parts[0] == "this");
        CHECK(parts[1].empty());
        CHECK(parts[2] == "is");
    }

    {
        const auto parts = JobStringUtils::split(std::string_view{" this "});

        REQUIRE(parts.size() == 3);
        CHECK(parts[0].empty());
        CHECK(parts[1] == "this");
        CHECK(parts[2].empty());
    }

    {
        const auto parts = JobStringUtils::split(std::string_view{});

        REQUIRE(parts.size() == 1);
        CHECK(parts[0].empty());
    }

    {
        const auto parts = JobStringUtils::split(std::string_view{"a:b:c"}, ':');

        REQUIRE(parts.size() == 3);
        CHECK(parts[0] == "a");
        CHECK(parts[1] == "b");
        CHECK(parts[2] == "c");
    }
}

TEST_CASE("JobStringUtils noHead honors each clean mode", "[core][string-utils][edge][clean]")
{
    CHECK(JobStringUtils::noHead(
              std::string_view{"   value"},
              JobStringCleanMode::Space) == "value");

    CHECK(JobStringUtils::noHead(
              std::string_view{"\r\nvalue"},
              JobStringCleanMode::Esc) == "value");

    CHECK(JobStringUtils::noHead(
              std::string_view{" \r\nvalue"},
              JobStringCleanMode::SpaceAndEsc) == "value");

    CHECK(JobStringUtils::noHead(
              std::string_view{"\t\tvalue"},
              JobStringCleanMode::Tabs) == "value");

    CHECK(JobStringUtils::noHead(
              std::string_view{" \t\r\nvalue"},
              JobStringCleanMode::All) == "value");
}

TEST_CASE("JobStringUtils noTail honors each clean mode", "[core][string-utils][edge][clean]")
{
    CHECK(JobStringUtils::noTail(
              std::string_view{"value   "},
              JobStringCleanMode::Space) == "value");

    CHECK(JobStringUtils::noTail(
              std::string_view{"value\r\n"},
              JobStringCleanMode::Esc) == "value");

    CHECK(JobStringUtils::noTail(
              std::string_view{"value\r\n   "},
              JobStringCleanMode::SpaceAndEsc) == "value");

    CHECK(JobStringUtils::noTail(
              std::string_view{"value\t\t"},
              JobStringCleanMode::Tabs) == "value");

    CHECK(JobStringUtils::noTail(
              std::string_view{"value \t\r\n"},
              JobStringCleanMode::All) == "value");
}

TEST_CASE("JobStringUtils cleaning preserves nonmatching characters", "[core][string-utils][edge][clean]")
{
    CHECK(JobStringUtils::noHead(
              std::string_view{"\tvalue"},
              JobStringCleanMode::Space) == "\tvalue");

    CHECK(JobStringUtils::noTail(
              std::string_view{"value\t"},
              JobStringCleanMode::Space) == "value\t");

    CHECK(JobStringUtils::noHead(std::string_view{}).empty());
    CHECK(JobStringUtils::noTail(std::string_view{}).empty());
}

TEST_CASE("JobStringUtils rejects non ASCII scalars", "[core][string-utils][edge][ascii]")
{
    const auto ascii = text::UnicodeScalar::from(0x7F);
    const auto nonAscii = text::UnicodeScalar::from(0x80);

    REQUIRE(ascii.has_value());
    REQUIRE(nonAscii.has_value());

    CHECK(JobStringUtils::toAscii(std::array{*ascii}).size() == 1);
    CHECK(JobStringUtils::toAscii(std::array{*nonAscii}).empty());
}

TEST_CASE("JobStringUtils rejects non ASCII input bytes", "[core][string-utils][edge][ascii]")
{
    constexpr std::array<std::uint8_t, 1> maximumAscii{0x7F};
    constexpr std::array<std::uint8_t, 1> firstNonAscii{0x80};

    const auto valid = JobStringUtils::fromAscii(maximumAscii);

    REQUIRE(valid.size() == 1);
    CHECK(valid[0].value() == 0x7F);

    CHECK(JobStringUtils::fromAscii(firstNonAscii).empty());
}

TEST_CASE("JobStringUtils handles empty Unicode input", "[core][string-utils][edge][unicode]")
{
    const std::array<text::UnicodeScalar, 0> scalars{};
    const std::array<std::uint8_t, 0> utf8{};
    const std::array<char16_t, 0> utf16{};
    const std::array<char32_t, 0> utf32{};

    CHECK(JobStringUtils::toAscii(scalars).empty());
    CHECK(JobStringUtils::toUtf8(scalars).empty());
    CHECK(JobStringUtils::toUtf16(scalars).empty());
    CHECK(JobStringUtils::toUtf32(scalars).empty());

    CHECK(JobStringUtils::fromAscii(utf8).empty());
    CHECK(JobStringUtils::fromUtf8(utf8).empty());
    CHECK(JobStringUtils::fromUtf16(utf16).empty());
    CHECK(JobStringUtils::fromUtf32(utf32).empty());
}

TEST_CASE("JobStringUtils rejects malformed UTF8", "[core][string-utils][edge][unicode][utf8]")
{
    constexpr std::array<std::uint8_t, 1> continuationOnly{
        0x80
    };

    constexpr std::array<std::uint8_t, 2> truncatedThreeByte{
        0xE2, 0x82
    };

    constexpr std::array<std::uint8_t, 2> overlong{
        0xC0, 0x80
    };

    CHECK(JobStringUtils::fromUtf8(continuationOnly).empty());
    CHECK(JobStringUtils::fromUtf8(truncatedThreeByte).empty());
    CHECK(JobStringUtils::fromUtf8(overlong).empty());
}

TEST_CASE("JobStringUtils rejects malformed UTF16", "[core][string-utils][edge][unicode][utf16]")
{
    constexpr std::array<char16_t, 1> loneHighSurrogate{
        0xD83D
    };

    constexpr std::array<char16_t, 1> loneLowSurrogate{
        0xDE80
    };

    CHECK(JobStringUtils::fromUtf16(loneHighSurrogate).empty());
    CHECK(JobStringUtils::fromUtf16(loneLowSurrogate).empty());
}

TEST_CASE("JobStringUtils rejects malformed UTF32", "[core][string-utils][edge][unicode][utf32]")
{
    constexpr std::array<char32_t, 1> aboveUnicodeMaximum{
        static_cast<char32_t>(0x110000)
    };

    constexpr std::array<char32_t, 1> surrogate{
        static_cast<char32_t>(0xD800)
    };

    CHECK(JobStringUtils::fromUtf32(aboveUnicodeMaximum).empty());
    CHECK(JobStringUtils::fromUtf32(surrogate).empty());
}