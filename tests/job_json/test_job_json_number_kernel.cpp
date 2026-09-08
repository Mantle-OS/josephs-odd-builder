#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
    #include "test_job_json_utils.h"
#endif


#include <cstdint>
#include <limits>
#include <string_view>

#include <job_json_number_kernel.h>

namespace {

template <typename T>
void requireParse(std::string_view text, T expected)
{
    T value{};

    REQUIRE(job::json::JsonNumberKernel::parse(text, value));
    REQUIRE(value == expected);
}

template <typename T>
void requireParseFailure(std::string_view text, T initial)
{
    T value = initial;

    REQUIRE_FALSE(job::json::JsonNumberKernel::parse(text, value));
    REQUIRE(value == initial);
}

} // namespace

TEST_CASE("JsonNumberKernel parses signed integers", "[job_json][number_kernel]")
{
    requireParse<std::int8_t>("0", 0);
    requireParse<std::int8_t>("42", 42);
    requireParse<std::int8_t>("-42", -42);

    requireParse<std::int16_t>("12345", 12345);
    requireParse<std::int16_t>("-12345", -12345);

    requireParse<std::int32_t>("123456789", 123456789);
    requireParse<std::int32_t>("-123456789", -123456789);

    requireParse<std::int64_t>("9223372036854775807", std::numeric_limits<std::int64_t>::max());
    requireParse<std::int64_t>("-9223372036854775808", std::numeric_limits<std::int64_t>::min());
}

TEST_CASE("JsonNumberKernel parses unsigned integers", "[job_json][number_kernel]")
{
    requireParse<std::uint8_t>("0", 0);
    requireParse<std::uint8_t>("255", 255);

    requireParse<std::uint16_t>("65535", 65535);
    requireParse<std::uint32_t>("4294967295", 4294967295U);

    requireParse<std::uint64_t>(
        "18446744073709551615",
        std::numeric_limits<std::uint64_t>::max());
}

TEST_CASE("JsonNumberKernel accepts exact signed integer boundaries", "[job_json][number_kernel]")
{
    requireParse<std::int8_t>("-128", std::numeric_limits<std::int8_t>::min());
    requireParse<std::int8_t>("127", std::numeric_limits<std::int8_t>::max());

    requireParse<std::int16_t>("-32768", std::numeric_limits<std::int16_t>::min());
    requireParse<std::int16_t>("32767", std::numeric_limits<std::int16_t>::max());

    requireParse<std::int32_t>("-2147483648", std::numeric_limits<std::int32_t>::min());
    requireParse<std::int32_t>("2147483647", std::numeric_limits<std::int32_t>::max());
}

TEST_CASE("JsonNumberKernel rejects signed integer overflow", "[job_json][number_kernel]")
{
    requireParseFailure<std::int8_t>("128", 7);
    requireParseFailure<std::int8_t>("-129", 7);

    requireParseFailure<std::int16_t>("32768", 7);
    requireParseFailure<std::int16_t>("-32769", 7);

    requireParseFailure<std::int32_t>("2147483648", 7);
    requireParseFailure<std::int32_t>("-2147483649", 7);

    requireParseFailure<std::int64_t>("9223372036854775808", 7);
    requireParseFailure<std::int64_t>("-9223372036854775809", 7);
}

TEST_CASE("JsonNumberKernel rejects unsigned integer overflow", "[job_json][number_kernel]")
{
    requireParseFailure<std::uint8_t>("256", 7);
    requireParseFailure<std::uint16_t>("65536", 7);
    requireParseFailure<std::uint32_t>("4294967296", 7);
    requireParseFailure<std::uint64_t>("18446744073709551616", 7);
}

TEST_CASE("JsonNumberKernel rejects negative values for unsigned integers", "[job_json][number_kernel]")
{
    requireParseFailure<std::uint8_t>("-1", 7);
    requireParseFailure<std::uint16_t>("-1", 7);
    requireParseFailure<std::uint32_t>("-1", 7);
    requireParseFailure<std::uint64_t>("-1", 7);
}

TEST_CASE("JsonNumberKernel parses floating point values", "[job_json][number_kernel]")
{
    requireParse<float>("0", 0.0f);
    requireParse<float>("1.5", 1.5f);
    requireParse<float>("-1.5", -1.5f);

    requireParse<double>("0", 0.0);
    requireParse<double>("123.456", 123.456);
    requireParse<double>("-123.456", -123.456);
}

TEST_CASE("JsonNumberKernel parses exponent floating point values", "[job_json][number_kernel]")
{
    requireParse<float>("1e3", 1000.0f);
    requireParse<float>("1E3", 1000.0f);
    requireParse<float>("1e-3", 0.001f);

    requireParse<double>("1e10", 1e10);
    requireParse<double>("1E-10", 1e-10);
    requireParse<double>("-2.5e4", -25000.0);
}

TEST_CASE("JsonNumberKernel parses negative zero", "[job_json][number_kernel]")
{
    float floatValue = 1.0f;
    double doubleValue = 1.0;

    REQUIRE(job::json::JsonNumberKernel::parse("-0", floatValue));
    REQUIRE(job::json::JsonNumberKernel::parse("-0", doubleValue));

    REQUIRE(floatValue == 0.0f);
    REQUIRE(doubleValue == 0.0);
}

TEST_CASE("JsonNumberKernel parseInteger parses directly", "[job_json][number_kernel]")
{
    std::int32_t signedValue{};
    std::uint32_t unsignedValue{};

    REQUIRE(job::json::JsonNumberKernel::parseInteger("-42", signedValue));
    REQUIRE(job::json::JsonNumberKernel::parseInteger("42", unsignedValue));

    REQUIRE(signedValue == -42);
    REQUIRE(unsignedValue == 42);
}

TEST_CASE("JsonNumberKernel parseFloat parses directly", "[job_json][number_kernel]")
{
    float floatValue{};
    double doubleValue{};

    REQUIRE(job::json::JsonNumberKernel::parseFloat("3.5", floatValue));
    REQUIRE(job::json::JsonNumberKernel::parseFloat("-6.25e2", doubleValue));

    REQUIRE(floatValue == 3.5f);
    REQUIRE(doubleValue == -625.0);
}

TEST_CASE("JsonNumberKernel rejects empty input", "[job_json][number_kernel]")
{
    requireParseFailure<std::int32_t>("", 42);
    requireParseFailure<std::uint64_t>("", 42);
    requireParseFailure<float>("", 42.0f);
    requireParseFailure<double>("", 42.0);
}

TEST_CASE("JsonNumberKernel rejects partially consumed integer input", "[job_json][number_kernel]")
{
    requireParseFailure<std::int32_t>("42cake", 7);
    requireParseFailure<std::int32_t>("123abc", 7);
    requireParseFailure<std::int32_t>("1.0", 7);
}

TEST_CASE("JsonNumberKernel rejects partially consumed floating input", "[job_json][number_kernel]")
{
    requireParseFailure<float>("1.2.3", 7.0f);
    requireParseFailure<double>("42cake", 7.0);
    requireParseFailure<double>("1e3x", 7.0);
}

TEST_CASE("JsonNumberKernel rejects whitespace around input", "[job_json][number_kernel]")
{
    requireParseFailure<std::int32_t>(" 42", 7);
    requireParseFailure<std::int32_t>("42 ", 7);

    requireParseFailure<double>(" 1.5", 7.0);
    requireParseFailure<double>("1.5 ", 7.0);
}

TEST_CASE("JsonNumberKernel rejects explicit plus for integer input", "[job_json][number_kernel]")
{
    requireParseFailure<std::int32_t>("+1", 7);
    requireParseFailure<std::uint32_t>("+1", 7);
}

TEST_CASE("JsonNumberKernel does not modify integer result on failure", "[job_json][number_kernel]")
{
    std::int32_t value = 123456;

    REQUIRE_FALSE(job::json::JsonNumberKernel::parse("not-a-number", value));
    REQUIRE(value == 123456);

    REQUIRE_FALSE(job::json::JsonNumberKernel::parse("2147483648", value));
    REQUIRE(value == 123456);
}

TEST_CASE("JsonNumberKernel does not modify floating result on failure", "[job_json][number_kernel]")
{
    double value = 123.456;

    REQUIRE_FALSE(job::json::JsonNumberKernel::parse("cake", value));
    REQUIRE(value == 123.456);

    REQUIRE_FALSE(job::json::JsonNumberKernel::parse("1.2.3", value));
    REQUIRE(value == 123.456);
}


#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JsonNumberKernel benchmarks", "[job_json][number_kernel][benchmark]")
{
    BENCHMARK("JsonNumberKernel int32")
    {
        const auto text = job::json::tests::benchmarkOpaque<std::string_view>("123456789");

        std::int32_t value{};
        job::json::JsonNumberKernel::parse(text, value);

        job::json::tests::benchmarkDoNotOptimize(value);
        return value;
    };

    BENCHMARK("JsonNumberKernel uint64")
    {
        const auto text = job::json::tests::benchmarkOpaque<std::string_view>("18446744073709551615");

        std::uint64_t value{};
        job::json::JsonNumberKernel::parse(text, value);

        job::json::tests::benchmarkDoNotOptimize(value);
        return value;
    };

    BENCHMARK("JsonNumberKernel float")
    {
        const auto text = job::json::tests::benchmarkOpaque<std::string_view>("123.456");

        float value{};
        job::json::JsonNumberKernel::parse(text, value);

        job::json::tests::benchmarkDoNotOptimize(value);
        return value;
    };

    BENCHMARK("JsonNumberKernel double exponent")
    {
        const auto text = job::json::tests::benchmarkOpaque<std::string_view>("-1.23456789e42");

        double value{};
        job::json::JsonNumberKernel::parse(text, value);

        job::json::tests::benchmarkDoNotOptimize(value);
        return value;
    };
}

#endif
