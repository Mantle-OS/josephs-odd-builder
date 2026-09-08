#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonParser parses root boolean true", "[job_json][parser][scalar]")
{
    bool value = false;

    job::json::JsonParser parser{"true"};

    REQUIRE(parser.parse(value));
    REQUIRE(value);
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser parses root boolean false", "[job_json][parser][scalar]")
{
    bool value = true;

    job::json::JsonParser parser{"false"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(value);
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser rejects boolean into integer", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{"true"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser parses root signed integer", "[job_json][parser][scalar]")
{
    std::int32_t value{};

    job::json::JsonParser parser{"-12345"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == -12345);
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser parses root unsigned integer", "[job_json][parser][scalar]")
{
    std::uint32_t value{};

    job::json::JsonParser parser{"12345"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == 12345);
}

TEST_CASE("JsonParser parses root float", "[job_json][parser][scalar]")
{
    float value{};

    job::json::JsonParser parser{"123.5"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == 123.5f);
}

TEST_CASE("JsonParser parses root double exponent", "[job_json][parser][scalar]")
{
    double value{};

    job::json::JsonParser parser{"1.25e3"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == 1250.0);
}

TEST_CASE("JsonParser rejects overflowing integer", "[job_json][parser][scalar]")
{
    std::int8_t value = 7;

    job::json::JsonParser parser{"128"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 7);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects negative unsigned integer", "[job_json][parser][scalar]")
{
    std::uint32_t value = 7;

    job::json::JsonParser parser{"-1"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 7);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser parses root enum", "[job_json][parser][scalar]")
{
    job::json::tests::DispatchMode value = job::json::tests::DispatchMode::Zero;

    job::json::JsonParser parser{"1"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == job::json::tests::DispatchMode::One);
}

TEST_CASE("JsonParser parses root byte", "[job_json][parser][scalar]")
{
    std::byte value{};

    job::json::JsonParser parser{"255"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::byte{255});
}

TEST_CASE("JsonParser parses root char32_t", "[job_json][parser][scalar]")
{
    char32_t value{};

    job::json::JsonParser parser{"1114111"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == static_cast<char32_t>(1114111));
}

TEST_CASE("JsonParser parses borrowed root string", "[job_json][parser][scalar]")
{
    std::string value;

    job::json::JsonParser parser{R"("Cake Court")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "Cake Court");
}

TEST_CASE("JsonParser parses borrowed root string view", "[job_json][parser][scalar]")
{
    constexpr std::string_view source = R"("Cake Court")";

    std::string_view value;

    job::json::JsonParser parser{source};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "Cake Court");
    REQUIRE(value.data() == source.data() + 1);
    REQUIRE(value.size() == 10);
}

TEST_CASE("JsonParser parses empty root string", "[job_json][parser][scalar]")
{
    std::string value = "before";

    job::json::JsonParser parser{R"("")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.empty());
}

TEST_CASE("JsonParser parses raw UTF8 root string", "[job_json][parser][scalar]")
{
    std::string value;

    job::json::JsonParser parser{R"("Grüße 世界 😀")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "Grüße 世界 😀");
}

TEST_CASE("JsonParser decodes escaped root string", "[job_json][parser][scalar]")
{
    std::string value;

    job::json::JsonParser parser{R"("Cake\nCourt")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "Cake\nCourt");
}

TEST_CASE("JsonParser decodes Unicode root string", "[job_json][parser][scalar]")
{
    std::string value;

    job::json::JsonParser parser{R"("\u004A\u004F\u0042")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "JOB");
}

TEST_CASE("JsonParser decodes surrogate pair root string", "[job_json][parser][scalar]")
{
    std::string value;

    job::json::JsonParser parser{R"("\uD83D\uDE00")"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == "😀");
}

TEST_CASE("JsonParser rejects escaped string into string view", "[job_json][parser][scalar]")
{
    std::string backing = "before";
    std::string_view value = backing;

    job::json::JsonParser parser{R"("Cake\nCourt")"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value == "before");
    REQUIRE(value.data() == backing.data());
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser parses null into optional", "[job_json][parser][scalar]")
{
    std::optional<int> value = 42;

    job::json::JsonParser parser{"null"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("JsonParser parses scalar into optional", "[job_json][parser][scalar]")
{
    std::optional<int> value;

    job::json::JsonParser parser{"42"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParser rejects null into scalar", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{"null"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidNull);
}

TEST_CASE("JsonParser ignores surrounding JSON whitespace", "[job_json][parser][scalar]")
{
    int value{};

    job::json::JsonParser parser{" \t\r\n 42 \n\t "};

    REQUIRE(parser.parse(value));
    REQUIRE(value == 42);
}

TEST_CASE("JsonParser rejects empty source", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{""};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects whitespace only source", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{" \t\r\n "};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects invalid token", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{"@"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects malformed number token", "[job_json][parser][scalar]")
{
    int value = 42;

    job::json::JsonParser parser{"123abc"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects malformed literal token", "[job_json][parser][scalar]")
{
    bool value = false;

    job::json::JsonParser parser{"truefalse"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE_FALSE(value);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects trailing JSON value", "[job_json][parser][scalar]")
{
    int value{};

    job::json::JsonParser parser{"42 true"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedToken);
}

TEST_CASE("JsonParser rejects trailing structural token", "[job_json][parser][scalar]")
{
    int value{};

    job::json::JsonParser parser{"42]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedToken);
}

TEST_CASE("JsonParser reports clean source after successful parse", "[job_json][parser][scalar]")
{
    int value{};

    job::json::JsonParser parser{"42"};

    REQUIRE(parser.source() == "42");
    REQUIRE(parser.parse(value));
    REQUIRE(parser.source() == "42");
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser exposes configured maximum depth", "[job_json][parser][scalar]")
{
    job::json::JsonParser parser{"42", 17};

    REQUIRE(parser.maxDepth() == 17);
}

TEST_CASE("JsonParser clears previous diagnostic before parsing", "[job_json][parser][scalar]")
{
    int first = 42;

    job::json::JsonParser parser{"true"};

    REQUIRE_FALSE(parser.parse(first));
    REQUIRE(parser.diagnostic().hasError());

    bool second = false;

    REQUIRE_FALSE(parser.parse(second));
}

