#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <job_json_parser_destination.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonParserDestination exposes backing value", "[job_json][parser_destination][scalar]")
{
    int value = 7;

    job::json::JsonParserDestination<int> destination{value};

    REQUIRE(&destination.value() == &value);
    REQUIRE(destination.value() == 7);

    destination.value() = 42;

    REQUIRE(value == 42);
}

TEST_CASE("JsonParserDestination assigns boolean", "[job_json][parser_destination][scalar]")
{
    bool value = false;

    job::json::JsonParserDestination<bool> destination{value};

    REQUIRE(destination.boolean(true));
    REQUIRE(value);

    REQUIRE(destination.boolean(false));
    REQUIRE_FALSE(value);
}

TEST_CASE("JsonParserDestination rejects boolean for non boolean", "[job_json][parser_destination][scalar]")
{
    int value = 42;

    job::json::JsonParserDestination<int> destination{value};

    REQUIRE_FALSE(destination.boolean(true));
    REQUIRE(value == 42);
}

TEST_CASE("JsonParserDestination parses signed integer", "[job_json][parser_destination][scalar]")
{
    std::int32_t value{};

    job::json::JsonParserDestination<std::int32_t> destination{value};

    REQUIRE(destination.number("-12345"));
    REQUIRE(value == -12345);
}

TEST_CASE("JsonParserDestination parses unsigned integer", "[job_json][parser_destination][scalar]")
{
    std::uint32_t value{};

    job::json::JsonParserDestination<std::uint32_t> destination{value};

    REQUIRE(destination.number("12345"));
    REQUIRE(value == 12345);
}

TEST_CASE("JsonParserDestination parses signed char", "[job_json][parser_destination][scalar]")
{
    signed char value{};

    job::json::JsonParserDestination<signed char> destination{value};

    REQUIRE(destination.number("-42"));
    REQUIRE(value == static_cast<signed char>(-42));
}

TEST_CASE("JsonParserDestination parses unsigned char", "[job_json][parser_destination][scalar]")
{
    unsigned char value{};

    job::json::JsonParserDestination<unsigned char> destination{value};

    REQUIRE(destination.number("200"));
    REQUIRE(value == static_cast<unsigned char>(200));
}

TEST_CASE("JsonParserDestination rejects negative unsigned integer", "[job_json][parser_destination][scalar]")
{
    std::uint32_t value = 99;

    job::json::JsonParserDestination<std::uint32_t> destination{value};

    REQUIRE_FALSE(destination.number("-1"));
    REQUIRE(value == 99);
}

TEST_CASE("JsonParserDestination preserves integer on malformed input", "[job_json][parser_destination][scalar]")
{
    std::int32_t value = 77;

    job::json::JsonParserDestination<std::int32_t> destination{value};

    REQUIRE_FALSE(destination.number("123abc"));
    REQUIRE(value == 77);
}

TEST_CASE("JsonParserDestination preserves integer on overflow", "[job_json][parser_destination][scalar]")
{
    std::int8_t value = 12;

    job::json::JsonParserDestination<std::int8_t> destination{value};

    REQUIRE_FALSE(destination.number("128"));
    REQUIRE(value == 12);
}

TEST_CASE("JsonParserDestination preserves unsigned integer on overflow", "[job_json][parser_destination][scalar]")
{
    std::uint8_t value = 12;

    job::json::JsonParserDestination<std::uint8_t> destination{value};

    REQUIRE_FALSE(destination.number("256"));
    REQUIRE(value == 12);
}

TEST_CASE("JsonParserDestination parses float", "[job_json][parser_destination][scalar]")
{
    float value{};

    job::json::JsonParserDestination<float> destination{value};

    REQUIRE(destination.number("123.5"));
    REQUIRE(value == 123.5f);
}

TEST_CASE("JsonParserDestination parses double exponent", "[job_json][parser_destination][scalar]")
{
    double value{};

    job::json::JsonParserDestination<double> destination{value};

    REQUIRE(destination.number("1.25e3"));
    REQUIRE(value == 1250.0);
}

TEST_CASE("JsonParserDestination preserves floating point value on failure", "[job_json][parser_destination][scalar]")
{
    double value = 42.5;

    job::json::JsonParserDestination<double> destination{value};

    REQUIRE_FALSE(destination.number("potato"));
    REQUIRE(value == 42.5);
}

TEST_CASE("JsonParserDestination parses enum underlying value", "[job_json][parser_destination][scalar]")
{
    job::json::tests::DispatchMode value = job::json::tests::DispatchMode::Zero;

    job::json::JsonParserDestination<job::json::tests::DispatchMode> destination{value};

    REQUIRE(destination.number("1"));
    REQUIRE(value == job::json::tests::DispatchMode::One);
}

TEST_CASE("JsonParserDestination parses maximum enum underlying value", "[job_json][parser_destination][scalar]")
{
    job::json::tests::DispatchMode value = job::json::tests::DispatchMode::Zero;

    job::json::JsonParserDestination<job::json::tests::DispatchMode> destination{value};

    REQUIRE(destination.number("255"));
    REQUIRE(value == job::json::tests::DispatchMode::Max);
}

TEST_CASE("JsonParserDestination preserves enum on underlying overflow", "[job_json][parser_destination][scalar]")
{
    job::json::tests::DispatchMode value = job::json::tests::DispatchMode::One;

    job::json::JsonParserDestination<job::json::tests::DispatchMode> destination{value};

    REQUIRE_FALSE(destination.number("256"));
    REQUIRE(value == job::json::tests::DispatchMode::One);
}

TEST_CASE("JsonParserDestination parses byte", "[job_json][parser_destination][scalar]")
{
    std::byte value{};

    job::json::JsonParserDestination<std::byte> destination{value};

    REQUIRE(destination.number("255"));
    REQUIRE(value == std::byte{255});
}

TEST_CASE("JsonParserDestination rejects byte overflow", "[job_json][parser_destination][scalar]")
{
    std::byte value{42};

    job::json::JsonParserDestination<std::byte> destination{value};

    REQUIRE_FALSE(destination.number("256"));
    REQUIRE(value == std::byte{42});
}

TEST_CASE("JsonParserDestination rejects negative byte", "[job_json][parser_destination][scalar]")
{
    std::byte value{42};

    job::json::JsonParserDestination<std::byte> destination{value};

    REQUIRE_FALSE(destination.number("-1"));
    REQUIRE(value == std::byte{42});
}

TEST_CASE("JsonParserDestination parses char8_t", "[job_json][parser_destination][scalar]")
{
    char8_t value{};

    job::json::JsonParserDestination<char8_t> destination{value};

    REQUIRE(destination.number("255"));
    REQUIRE(value == static_cast<char8_t>(255));
}

TEST_CASE("JsonParserDestination rejects char8_t overflow", "[job_json][parser_destination][scalar]")
{
    char8_t value = static_cast<char8_t>(42);

    job::json::JsonParserDestination<char8_t> destination{value};

    REQUIRE_FALSE(destination.number("256"));
    REQUIRE(value == static_cast<char8_t>(42));
}

TEST_CASE("JsonParserDestination parses char16_t", "[job_json][parser_destination][scalar]")
{
    char16_t value{};

    job::json::JsonParserDestination<char16_t> destination{value};

    REQUIRE(destination.number("65535"));
    REQUIRE(value == static_cast<char16_t>(65535));
}

TEST_CASE("JsonParserDestination rejects char16_t overflow", "[job_json][parser_destination][scalar]")
{
    char16_t value = static_cast<char16_t>(42);

    job::json::JsonParserDestination<char16_t> destination{value};

    REQUIRE_FALSE(destination.number("65536"));
    REQUIRE(value == static_cast<char16_t>(42));
}

TEST_CASE("JsonParserDestination parses char32_t", "[job_json][parser_destination][scalar]")
{
    char32_t value{};

    job::json::JsonParserDestination<char32_t> destination{value};

    REQUIRE(destination.number("1114111"));
    REQUIRE(value == static_cast<char32_t>(1114111));
}

TEST_CASE("JsonParserDestination parses wchar_t maximum", "[job_json][parser_destination][scalar]")
{
    wchar_t value{};

    job::json::JsonParserDestination<wchar_t> destination{value};

    constexpr auto maximum = std::numeric_limits<wchar_t>::max();
    const std::string text = std::to_string(static_cast<long long>(maximum));

    REQUIRE(destination.number(text));
    REQUIRE(value == maximum);
}

TEST_CASE("JsonParserDestination handles signed wchar_t minimum", "[job_json][parser_destination][scalar]")
{
    if constexpr (std::numeric_limits<wchar_t>::is_signed) {
        wchar_t value{};

        job::json::JsonParserDestination<wchar_t> destination{value};

        constexpr auto minimum = std::numeric_limits<wchar_t>::min();
        const std::string text = std::to_string(static_cast<long long>(minimum));

        REQUIRE(destination.number(text));
        REQUIRE(value == minimum);
    }
}

TEST_CASE("JsonParserDestination rejects wchar_t overflow", "[job_json][parser_destination][scalar]")
{
    wchar_t value = static_cast<wchar_t>(42);

    job::json::JsonParserDestination<wchar_t> destination{value};

    REQUIRE_FALSE(destination.number("18446744073709551615"));
    REQUIRE(value == static_cast<wchar_t>(42));
}

TEST_CASE("JsonParserDestination rejects number for boolean", "[job_json][parser_destination][scalar]")
{
    bool value = true;

    job::json::JsonParserDestination<bool> destination{value};

    REQUIRE_FALSE(destination.number("1"));
    REQUIRE(value);
}

TEST_CASE("JsonParserDestination rejects number for owned string", "[job_json][parser_destination][scalar]")
{
    std::string value = "cake";

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE_FALSE(destination.number("42"));
    REQUIRE(value == "cake");
}

TEST_CASE("JsonParserDestination rejects null for scalar", "[job_json][parser_destination][scalar]")
{
    int value = 42;

    job::json::JsonParserDestination<int> destination{value};

    REQUIRE_FALSE(destination.null());
    REQUIRE(value == 42);
}

