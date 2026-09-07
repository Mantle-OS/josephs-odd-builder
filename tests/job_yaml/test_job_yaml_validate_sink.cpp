#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_object_sink.h>
#include <job_yaml_sink.h>
#include <job_yaml_validate_sink.h>

#include "../tests-fast-math-workaround.h"
#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace job::yaml::tests {

// ========================================
// Scalar boolean validation
// ========================================

TEST_CASE("YamlValidateSink accepts valid boolean scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<bool>("true"));
    REQUIRE(YamlValidateSink::scalar<bool>("false"));
    REQUIRE(YamlValidateSink::scalar<bool>("TRUE"));
    REQUIRE(YamlValidateSink::scalar<bool>("FALSE"));
}

TEST_CASE("YamlValidateSink rejects invalid boolean scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>("yes"));
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>("no"));
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>("1"));
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>(""));
}

// ========================================
// Scalar signed integer validation
// ========================================

TEST_CASE("YamlValidateSink accepts valid signed integer scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<int>("0"));
    REQUIRE(YamlValidateSink::scalar<int>("42"));
    REQUIRE(YamlValidateSink::scalar<int>("-42"));
}

TEST_CASE("YamlValidateSink accepts signed integer boundaries",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::int32_t>("-2147483648"));
    REQUIRE(YamlValidateSink::scalar<std::int32_t>("2147483647"));
}

TEST_CASE("YamlValidateSink rejects signed integer overflow",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<std::int32_t>("-2147483649"));
    REQUIRE_FALSE(YamlValidateSink::scalar<std::int32_t>("2147483648"));
}

TEST_CASE("YamlValidateSink rejects malformed signed integer",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<int>("hello"));
    REQUIRE_FALSE(YamlValidateSink::scalar<int>("12x"));
    REQUIRE_FALSE(YamlValidateSink::scalar<int>(""));
}

// ========================================
// Scalar unsigned integer validation
// ========================================

TEST_CASE("YamlValidateSink accepts valid unsigned integer scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::uint32_t>("0"));
    REQUIRE(YamlValidateSink::scalar<std::uint32_t>("42"));
    REQUIRE(YamlValidateSink::scalar<std::uint32_t>("0xFF"));
    REQUIRE(YamlValidateSink::scalar<std::uint32_t>("0o755"));
}

TEST_CASE("YamlValidateSink accepts unsigned integer maximum",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::uint32_t>("4294967295"));
}

TEST_CASE("YamlValidateSink rejects unsigned integer overflow",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<std::uint32_t>("4294967296"));
}

TEST_CASE("YamlValidateSink rejects negative unsigned integer",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<std::uint32_t>("-1"));
}

// ========================================
// Scalar floating point validation
// ========================================

TEST_CASE("YamlValidateSink accepts valid floating point scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<float>("1.5"));
    REQUIRE(YamlValidateSink::scalar<double>("-2.25"));
    REQUIRE(YamlValidateSink::scalar<double>("1e3"));
}

TEST_CASE("YamlValidateSink accepts YAML floating point specials",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<float>(".inf"));
    REQUIRE(YamlValidateSink::scalar<float>("-.inf"));
    REQUIRE(YamlValidateSink::scalar<double>(".nan"));
    REQUIRE(YamlValidateSink::scalar<double>(".INF"));
    REQUIRE(YamlValidateSink::scalar<double>("-.INF"));
    REQUIRE(YamlValidateSink::scalar<double>(".NAN"));
}

TEST_CASE("YamlValidateSink rejects malformed floating point scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<double>("hello"));
    REQUIRE_FALSE(YamlValidateSink::scalar<double>("1.2.3"));
    REQUIRE_FALSE(YamlValidateSink::scalar<double>("inf"));
    REQUIRE_FALSE(YamlValidateSink::scalar<double>("nan"));
    REQUIRE_FALSE(YamlValidateSink::scalar<double>(""));
}

// ========================================
// Scalar owned string validation
// ========================================

TEST_CASE("YamlValidateSink accepts owned string scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::string>("Joseph"));
    REQUIRE(YamlValidateSink::scalar<std::string>("true"));
    REQUIRE(YamlValidateSink::scalar<std::string>("42"));
    REQUIRE(YamlValidateSink::scalar<std::string>("null"));
}

TEST_CASE("YamlValidateSink accepts empty owned string scalar",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::string>(""));
}

TEST_CASE("YamlValidateSink accepts arbitrary owned string payload",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::scalar<std::string>("  Joseph  "));
    REQUIRE(YamlValidateSink::scalar<std::string>("hello\\nworld"));
    REQUIRE(YamlValidateSink::scalar<std::string>("\"Joseph\""));
}

// ========================================
// String view destination policy
// ========================================

TEST_CASE("YamlValidateSink rejects string_view parse destination",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<std::string_view>("Joseph"));
    REQUIRE_FALSE(YamlValidateSink::scalar<std::string_view>(""));
}

// ========================================
// Whitespace ownership
// ========================================

TEST_CASE("YamlValidateSink does not trim boolean input",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>(" true"));
    REQUIRE_FALSE(YamlValidateSink::scalar<bool>("true "));
}

TEST_CASE("YamlValidateSink does not trim integer input",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<int>(" 42"));
    REQUIRE_FALSE(YamlValidateSink::scalar<int>("42 "));
}

TEST_CASE("YamlValidateSink does not trim floating point input",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(YamlValidateSink::scalar<double>(" 2.5"));
    REQUIRE_FALSE(YamlValidateSink::scalar<double>("2.5 "));
}

// ========================================
// Non-null-terminated scalar input
// ========================================

TEST_CASE("YamlValidateSink accepts non null terminated integer string_view",
          "[job_yaml][validate_sink]")
{
    constexpr char source[] = {
        'x',
        '1', '2', '3',
        'x'
    };

    const std::string_view value{source + 1, 3};

    REQUIRE(YamlValidateSink::scalar<int>(value));
}

TEST_CASE("YamlValidateSink respects scalar string_view boundary",
          "[job_yaml][validate_sink]")
{
    constexpr char source[] = "42bad";
    const std::string_view valid{source, 2};
    const std::string_view invalid{source, 5};

    REQUIRE(YamlValidateSink::scalar<int>(valid));
    REQUIRE_FALSE(YamlValidateSink::scalar<int>(invalid));
}

// ========================================
// Reflected member validation
// ========================================

TEST_CASE("YamlValidateSink validates signed integer member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("count", "42"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("count", "-42"));
}

TEST_CASE("YamlValidateSink validates unsigned integer member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("size", "1024"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("size", "0xFF"));
}

TEST_CASE("YamlValidateSink validates boolean member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("enabled", "true"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("enabled", "false"));
}

TEST_CASE("YamlValidateSink validates float member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("scale", "1.5"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("scale", ".inf"));
}

TEST_CASE("YamlValidateSink validates double member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("ratio", "2.25"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("ratio", "-.inf"));
}

TEST_CASE("YamlValidateSink validates owned string member",
          "[job_yaml][validate_sink]")
{
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("name", "Joseph"));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("name", ""));
    REQUIRE(YamlValidateSink::member<ObjectReaderObject>("name", "42"));
}

// ========================================
// Reflected member failures
// ========================================

TEST_CASE("YamlValidateSink rejects unknown reflected member",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("missing", "42"));
}

TEST_CASE("YamlValidateSink rejects empty reflected member key",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("", "42"));
}

TEST_CASE("YamlValidateSink rejects invalid reflected integer value",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("count", "bad"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>(
            "count",
            "2147483648"));
}

TEST_CASE("YamlValidateSink rejects invalid reflected unsigned value",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("size", "-1"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>(
            "size",
            "4294967296"));
}

TEST_CASE("YamlValidateSink rejects invalid reflected boolean value",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("enabled", "yes"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("enabled", "1"));
}

TEST_CASE("YamlValidateSink rejects invalid reflected floating point value",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("scale", "bad"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>("ratio", "1.2.3"));
}

// ========================================
// Unsupported reflected destination
// ========================================

TEST_CASE("YamlValidateSink accepts supported member in partially unsupported object",
          "[job_yaml][validate_sink]")
{
    REQUIRE(
        YamlValidateSink::member<ValidateUnsupportedObject>("count", "42"));
}

TEST_CASE("YamlValidateSink rejects unsupported reflected member type",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ValidateUnsupportedObject>("values", "42"));
}

TEST_CASE("YamlValidateSink distinguishes unsupported and unknown member",
          "[job_yaml][validate_sink]")
{
    REQUIRE_FALSE(
        YamlValidateSink::member<ValidateUnsupportedObject>("values", "42"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ValidateUnsupportedObject>("missing", "42"));
}

// ========================================
// No object instance required
// ========================================

TEST_CASE("YamlValidateSink validates reflected schema without object instance",
          "[job_yaml][validate_sink]")
{
    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>("count", "42"));

    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>("enabled", "true"));

    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>("name", "Joseph"));
}

// ========================================
// Scalar semantic agreement with YamlSink
// ========================================

TEST_CASE("YamlValidateSink agrees with YamlSink for valid integer scalar",
          "[job_yaml][validate_sink]")
{
    int destination{};

    const bool validated =
        YamlValidateSink::scalar<int>("-123");

    const bool assigned =
        YamlSink::scalar(destination, "-123");

    REQUIRE(validated == assigned);
    REQUIRE(validated);
    REQUIRE(destination == -123);
}

TEST_CASE("YamlValidateSink agrees with YamlSink for invalid integer scalar",
          "[job_yaml][validate_sink]")
{
    int destination = 17;

    const bool validated =
        YamlValidateSink::scalar<int>("bad");

    const bool assigned =
        YamlSink::scalar(destination, "bad");

    REQUIRE(validated == assigned);
    REQUIRE_FALSE(validated);
    REQUIRE(destination == 17);
}

TEST_CASE("YamlValidateSink agrees with YamlSink for boolean scalar",
          "[job_yaml][validate_sink]")
{
    bool destination = false;

    REQUIRE(
        YamlValidateSink::scalar<bool>("true") ==
        YamlSink::scalar(destination, "true"));

    REQUIRE(destination);
}

TEST_CASE("YamlValidateSink agrees with YamlSink for floating point scalar",
          "[job_yaml][validate_sink]")
{
    double destination{};

    REQUIRE(
        YamlValidateSink::scalar<double>("3.125") ==
        YamlSink::scalar(destination, "3.125"));

    REQUIRE(destination == 3.125);
}

TEST_CASE("YamlValidateSink agrees with YamlSink for owned string scalar",
          "[job_yaml][validate_sink]")
{
    std::string destination;

    REQUIRE(
        YamlValidateSink::scalar<std::string>("Joseph") ==
        YamlSink::scalar(destination, "Joseph"));

    REQUIRE(destination == "Joseph");
}

TEST_CASE("YamlValidateSink agrees with YamlSink for string_view rejection",
          "[job_yaml][validate_sink]")
{
    std::string_view destination = "old";

    REQUIRE(
        YamlValidateSink::scalar<std::string_view>("Joseph") ==
        YamlSink::scalar(destination, "Joseph"));

    REQUIRE(destination == "old");
}

// ========================================
// Member semantic agreement with ObjectSink
// ========================================

TEST_CASE("YamlValidateSink agrees with YamlObjectSink for valid member",
          "[job_yaml][validate_sink]")
{
    ObjectReaderObject object{};

    const bool validated =
        YamlValidateSink::member<ObjectReaderObject>("count", "42");

    const bool assigned =
        YamlObjectSink::member(object, "count", "42");

    REQUIRE(validated == assigned);
    REQUIRE(validated);
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlValidateSink agrees with YamlObjectSink for invalid member value",
          "[job_yaml][validate_sink]")
{
    ObjectReaderObject object{
        .count = 17,
    };

    const bool validated =
        YamlValidateSink::member<ObjectReaderObject>("count", "bad");

    const bool assigned =
        YamlObjectSink::member(object, "count", "bad");

    REQUIRE(validated == assigned);
    REQUIRE_FALSE(validated);
    REQUIRE(object.count == 17);
}

TEST_CASE("YamlValidateSink agrees with YamlObjectSink for unknown member",
          "[job_yaml][validate_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>("missing", "42") ==
        YamlObjectSink::member(object, "missing", "42"));
}

// ========================================
// Reflected key string_view boundaries
// ========================================

TEST_CASE("YamlValidateSink accepts non null terminated reflected key",
          "[job_yaml][validate_sink]")
{
    constexpr char source[] = {
        'x',
        'c', 'o', 'u', 'n', 't',
        'x'
    };

    const std::string_view key{source + 1, 5};

    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>(key, "42"));
}

TEST_CASE("YamlValidateSink respects reflected key string_view boundary",
          "[job_yaml][validate_sink]")
{
    constexpr char source[] = "count-extra";

    const std::string_view valid{source, 5};
    const std::string_view invalid{source, sizeof(source) - 1};

    REQUIRE(
        YamlValidateSink::member<ObjectReaderObject>(valid, "42"));

    REQUIRE_FALSE(
        YamlValidateSink::member<ObjectReaderObject>(invalid, "42"));
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlValidateSink scalar benchmarks",
          "[job_yaml][validate_sink][benchmark]")
{
    std::string_view integerValue = "42";
    std::string_view booleanValue = "true";
    std::string_view doubleValue = "3.1415926535";

    benchmarkDoNotOptimize(integerValue);
    benchmarkDoNotOptimize(booleanValue);
    benchmarkDoNotOptimize(doubleValue);

    BENCHMARK("YamlValidateSink integer")
    {
        benchmarkDoNotOptimize(integerValue);

        const bool result =
            YamlValidateSink::scalar<int>(integerValue);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlSink integer")
    {
        int destination{};

        benchmarkDoNotOptimize(integerValue);

        const bool result =
            YamlSink::scalar(destination, integerValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("YamlValidateSink boolean")
    {
        benchmarkDoNotOptimize(booleanValue);

        const bool result =
            YamlValidateSink::scalar<bool>(booleanValue);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlSink boolean")
    {
        bool destination = false;

        benchmarkDoNotOptimize(booleanValue);

        const bool result =
            YamlSink::scalar(destination, booleanValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("YamlValidateSink double")
    {
        benchmarkDoNotOptimize(doubleValue);

        const bool result =
            YamlValidateSink::scalar<double>(doubleValue);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlSink double")
    {
        double destination{};

        benchmarkDoNotOptimize(doubleValue);

        const bool result =
            YamlSink::scalar(destination, doubleValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };
}

TEST_CASE("YamlValidateSink reflected member benchmark",
          "[job_yaml][validate_sink][benchmark]")
{
    std::string_view key = "count";
    std::string_view value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlValidateSink integer member")
    {
        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlValidateSink::member<ObjectReaderObject>(key, value);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlObjectSink integer member")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlObjectSink::member(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };

    BENCHMARK("YamlObjectReader integer member")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlValidateSink complete schema benchmark",
          "[job_yaml][validate_sink][benchmark]")
{
    std::string_view countKey = "count";
    std::string_view countValue = "42";

    std::string_view sizeKey = "size";
    std::string_view sizeValue = "1024";

    std::string_view enabledKey = "enabled";
    std::string_view enabledValue = "true";

    std::string_view scaleKey = "scale";
    std::string_view scaleValue = "1.5";

    std::string_view ratioKey = "ratio";
    std::string_view ratioValue = "2.25";

    std::string_view nameKey = "name";
    std::string_view nameValue = "Joseph";

    benchmarkDoNotOptimize(countKey);
    benchmarkDoNotOptimize(countValue);
    benchmarkDoNotOptimize(sizeKey);
    benchmarkDoNotOptimize(sizeValue);
    benchmarkDoNotOptimize(enabledKey);
    benchmarkDoNotOptimize(enabledValue);
    benchmarkDoNotOptimize(scaleKey);
    benchmarkDoNotOptimize(scaleValue);
    benchmarkDoNotOptimize(ratioKey);
    benchmarkDoNotOptimize(ratioValue);
    benchmarkDoNotOptimize(nameKey);
    benchmarkDoNotOptimize(nameValue);

    BENCHMARK("YamlValidateSink six member schema")
    {
        bool valid = true;

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            countKey,
            countValue);

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            sizeKey,
            sizeValue);

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            enabledKey,
            enabledValue);

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            scaleKey,
            scaleValue);

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            ratioKey,
            ratioValue);

        valid &= YamlValidateSink::member<ObjectReaderObject>(
            nameKey,
            nameValue);

        benchmarkDoNotOptimize(valid);

        return valid;
    };

    BENCHMARK("YamlObjectSink six member object")
    {
        ObjectReaderObject object{};

        YamlObjectSink::member(object, countKey, countValue);
        YamlObjectSink::member(object, sizeKey, sizeValue);
        YamlObjectSink::member(object, enabledKey, enabledValue);
        YamlObjectSink::member(object, scaleKey, scaleValue);
        YamlObjectSink::member(object, ratioKey, ratioValue);
        YamlObjectSink::member(object, nameKey, nameValue);

        benchmarkClobber(object);

        return object.count +
               static_cast<int>(object.size) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.scale) +
               static_cast<int>(object.ratio) +
               static_cast<int>(object.name.size());
    };
}

#endif

} // namespace job::yaml::tests