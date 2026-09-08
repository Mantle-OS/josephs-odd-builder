#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <job_json_emitter.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonEmitter emits boolean true", "[job_json][emitter][scalar]")
{
    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(true, output));
    REQUIRE(output == "true");
}

TEST_CASE("JsonEmitter emits boolean false", "[job_json][emitter][scalar]")
{
    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(false, output));
    REQUIRE(output == "false");
}

TEST_CASE("JsonEmitter emits signed integer", "[job_json][emitter][scalar]")
{
    std::string output;
    const std::int32_t value = -12345;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "-12345");
}

TEST_CASE("JsonEmitter emits unsigned integer", "[job_json][emitter][scalar]")
{
    std::string output;
    const std::uint32_t value = 12345;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "12345");
}

TEST_CASE("JsonEmitter emits signed integer minimum", "[job_json][emitter][scalar]")
{
    std::string output;
    const auto value = std::numeric_limits<std::int64_t>::min();

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "-9223372036854775808");
}

TEST_CASE("JsonEmitter emits unsigned integer maximum", "[job_json][emitter][scalar]")
{
    std::string output;
    const auto value = std::numeric_limits<std::uint64_t>::max();

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "18446744073709551615");
}

TEST_CASE("JsonEmitter emits signed char numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const signed char value = -42;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "-42");
}

TEST_CASE("JsonEmitter emits unsigned char numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const unsigned char value = 255;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "255");
}

TEST_CASE("JsonEmitter emits float", "[job_json][emitter][scalar]")
{
    std::string output;
    const float value = 123.5f;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "123.5");
}

TEST_CASE("JsonEmitter emits negative float", "[job_json][emitter][scalar]")
{
    std::string output;
    const float value = -0.5f;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "-0.5");
}

TEST_CASE("JsonEmitter emits double", "[job_json][emitter][scalar]")
{
    std::string output;
    const double value = 1250.25;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "1250.25");
}

TEST_CASE("JsonEmitter emits zero float", "[job_json][emitter][scalar]")
{
    std::string output;
    const double value = 0.0;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "0");
}

TEST_CASE("JsonEmitter emits negative zero float as valid JSON number", "[job_json][emitter][scalar]")
{
    std::string output;
    const double value = -0.0;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "-0");
}

TEST_CASE("JsonEmitter rejects positive infinity", "[job_json][emitter][scalar]")
{
    std::string output = "before";
    const double value = std::numeric_limits<double>::infinity();

    REQUIRE_FALSE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "before");
}

TEST_CASE("JsonEmitter rejects negative infinity", "[job_json][emitter][scalar]")
{
    std::string output = "before";
    const double value = -std::numeric_limits<double>::infinity();

    REQUIRE_FALSE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "before");
}

TEST_CASE("JsonEmitter rejects NaN", "[job_json][emitter][scalar]")
{
    std::string output = "before";
    const double value = std::numeric_limits<double>::quiet_NaN();

    REQUIRE_FALSE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "before");
}

TEST_CASE("JsonEmitter emits enum using underlying value", "[job_json][emitter][scalar]")
{
    std::string output;
    const auto value = job::json::tests::DispatchMode::One;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "1");
}

TEST_CASE("JsonEmitter emits enum maximum underlying value", "[job_json][emitter][scalar]")
{
    std::string output;
    const auto value = job::json::tests::DispatchMode::Max;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "255");
}

TEST_CASE("JsonEmitter emits byte numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const std::byte value{255};

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "255");
}

TEST_CASE("JsonEmitter emits char8_t numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const char8_t value = static_cast<char8_t>(255);

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "255");
}

TEST_CASE("JsonEmitter emits char16_t numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const char16_t value = static_cast<char16_t>(65535);

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "65535");
}

TEST_CASE("JsonEmitter emits char32_t numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const char32_t value = static_cast<char32_t>(1114111);

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "1114111");
}

TEST_CASE("JsonEmitter emits wchar_t numerically", "[job_json][emitter][scalar]")
{
    std::string output;
    const wchar_t value = static_cast<wchar_t>(42);

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter appends to existing sink", "[job_json][emitter][scalar]")
{
    std::string output = "prefix:";

    REQUIRE(job::json::JsonEmitter::emit(42, output));
    REQUIRE(output == "prefix:42");
}

TEST_CASE("JsonEmitter emits multiple scalar values into same sink", "[job_json][emitter][scalar]")
{
    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(42, output));
    REQUIRE(job::json::JsonEmitter::emit(true, output));
    REQUIRE(job::json::JsonEmitter::emit(-7, output));

    REQUIRE(output == "42true-7");
}

