#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_sink.h>

#include "../tests-fast-math-workaround.h"
#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace job::yaml::tests {

// ========================================
// Scalar boolean destination
// ========================================

TEST_CASE("YamlSink reads true into boolean destination", "[job_yaml][sink]")
{
    bool destination = false;

    REQUIRE(YamlSink::scalar(destination, "true"));
    REQUIRE(destination);
}

TEST_CASE("YamlSink reads false into boolean destination", "[job_yaml][sink]")
{
    bool destination = true;

    REQUIRE(YamlSink::scalar(destination, "FALSE"));
    REQUIRE_FALSE(destination);
}

TEST_CASE("YamlSink rejects invalid boolean without changing destination",
          "[job_yaml][sink]")
{
    bool destination = true;

    REQUIRE_FALSE(YamlSink::scalar(destination, "yes"));
    REQUIRE(destination);
}

TEST_CASE("YamlSink routes boolean before integer handling", "[job_yaml][sink]")
{
    bool destination = true;

    REQUIRE_FALSE(YamlSink::scalar(destination, "1"));
    REQUIRE(destination);
}

// ========================================
// Scalar signed integer destination
// ========================================

TEST_CASE("YamlSink reads signed integer destination", "[job_yaml][sink]")
{
    int destination{};

    REQUIRE(YamlSink::scalar(destination, "-42"));
    REQUIRE(destination == -42);
}

TEST_CASE("YamlSink overwrites signed integer destination", "[job_yaml][sink]")
{
    int destination = 7;

    REQUIRE(YamlSink::scalar(destination, "99"));
    REQUIRE(destination == 99);
}

TEST_CASE("YamlSink reads signed integer boundaries", "[job_yaml][sink]")
{
    int destination{};

    REQUIRE(YamlSink::scalar(destination, "-2147483648"));
    REQUIRE(destination == std::numeric_limits<int>::min());

    REQUIRE(YamlSink::scalar(destination, "2147483647"));
    REQUIRE(destination == std::numeric_limits<int>::max());
}

TEST_CASE("YamlSink rejects signed integer overflow without changing destination",
          "[job_yaml][sink]")
{
    int destination = 17;

    REQUIRE_FALSE(YamlSink::scalar(destination, "2147483648"));
    REQUIRE(destination == 17);

    REQUIRE_FALSE(YamlSink::scalar(destination, "-2147483649"));
    REQUIRE(destination == 17);
}

TEST_CASE("YamlSink rejects malformed signed integer without changing destination",
          "[job_yaml][sink]")
{
    int destination = 17;

    REQUIRE_FALSE(YamlSink::scalar(destination, "hello"));
    REQUIRE(destination == 17);
}

// ========================================
// Scalar unsigned integer destination
// ========================================

TEST_CASE("YamlSink reads unsigned integer destination", "[job_yaml][sink]")
{
    std::uint32_t destination{};

    REQUIRE(YamlSink::scalar(destination, "123"));
    REQUIRE(destination == 123U);
}

TEST_CASE("YamlSink reads hexadecimal unsigned integer destination",
          "[job_yaml][sink]")
{
    std::uint32_t destination{};

    REQUIRE(YamlSink::scalar(destination, "0xFF"));
    REQUIRE(destination == 255U);
}

TEST_CASE("YamlSink reads octal unsigned integer destination",
          "[job_yaml][sink]")
{
    std::uint32_t destination{};

    REQUIRE(YamlSink::scalar(destination, "0o755"));
    REQUIRE(destination == 493U);
}

TEST_CASE("YamlSink reads unsigned integer maximum", "[job_yaml][sink]")
{
    std::uint32_t destination{};

    REQUIRE(YamlSink::scalar(destination, "4294967295"));
    REQUIRE(destination == std::numeric_limits<std::uint32_t>::max());
}

TEST_CASE("YamlSink rejects negative unsigned integer without changing destination",
          "[job_yaml][sink]")
{
    std::uint32_t destination = 23U;

    REQUIRE_FALSE(YamlSink::scalar(destination, "-1"));
    REQUIRE(destination == 23U);
}

TEST_CASE("YamlSink rejects unsigned integer overflow without changing destination",
          "[job_yaml][sink]")
{
    std::uint32_t destination = 23U;

    REQUIRE_FALSE(YamlSink::scalar(destination, "4294967296"));
    REQUIRE(destination == 23U);
}

// ========================================
// Scalar floating point destination
// ========================================

TEST_CASE("YamlSink reads float destination", "[job_yaml][sink]")
{
    float destination{};

    REQUIRE(YamlSink::scalar(destination, "1.5"));
    REQUIRE(destination == 1.5F);
}

TEST_CASE("YamlSink reads double destination", "[job_yaml][sink]")
{
    double destination{};

    REQUIRE(YamlSink::scalar(destination, "2.25"));
    REQUIRE(destination == 2.25);
}

TEST_CASE("YamlSink reads scientific notation into floating point destination",
          "[job_yaml][sink]")
{
    double destination{};

    REQUIRE(YamlSink::scalar(destination, "1e3"));
    REQUIRE(destination == 1000.0);
}

TEST_CASE("YamlSink rejects malformed floating point without changing destination",
          "[job_yaml][sink]")
{
    double destination = 2.5;

    REQUIRE_FALSE(YamlSink::scalar(destination, "1.2.3"));
    REQUIRE(destination == 2.5);
}

// ========================================
// Special floating point values
// ========================================

TEST_CASE("YamlSink reads positive infinity into floating point destination",
          "[job_yaml][sink][fast_math]")
{
    float destination{};

    REQUIRE(YamlSink::scalar(destination, ".inf"));
    REQUIRE(isSafePositiveInfinity(destination));
}

TEST_CASE("YamlSink reads negative infinity into floating point destination",
          "[job_yaml][sink][fast_math]")
{
    double destination{};

    REQUIRE(YamlSink::scalar(destination, "-.INF"));
    REQUIRE(isSafeNegativeInfinity(destination));
}

TEST_CASE("YamlSink reads NaN into floating point destination",
          "[job_yaml][sink][fast_math]")
{
    double destination{};

    REQUIRE(YamlSink::scalar(destination, ".nan"));
    REQUIRE(isSafeNaN(destination));
}

// ========================================
// Owned string destination
// ========================================

TEST_CASE("YamlSink reads owned string destination", "[job_yaml][sink]")
{
    std::string destination;

    REQUIRE(YamlSink::scalar(destination, "Joseph"));
    REQUIRE(destination == "Joseph");
}

TEST_CASE("YamlSink overwrites owned string destination", "[job_yaml][sink]")
{
    std::string destination = "old";

    REQUIRE(YamlSink::scalar(destination, "new"));
    REQUIRE(destination == "new");
}

TEST_CASE("YamlSink accepts empty owned string destination", "[job_yaml][sink]")
{
    std::string destination = "not empty";

    REQUIRE(YamlSink::scalar(destination, ""));
    REQUIRE(destination.empty());
}

TEST_CASE("YamlSink preserves exact owned string input", "[job_yaml][sink]")
{
    std::string destination;

    REQUIRE(YamlSink::scalar(destination, "  Joseph  "));
    REQUIRE(destination == "  Joseph  ");
}

TEST_CASE("YamlSink does not interpret string escapes", "[job_yaml][sink]")
{
    std::string destination;

    REQUIRE(YamlSink::scalar(destination, "hello\\nworld"));
    REQUIRE(destination == "hello\\nworld");
}

TEST_CASE("YamlSink does not remove string quotes", "[job_yaml][sink]")
{
    std::string destination;

    REQUIRE(YamlSink::scalar(destination, "\"Joseph\""));
    REQUIRE(destination == "\"Joseph\"");
}

// ========================================
// Binary-safe owned strings
// ========================================

TEST_CASE("YamlSink preserves embedded null in owned string destination",
          "[job_yaml][sink]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};
    std::string destination;

    REQUIRE(YamlSink::scalar(destination, value));
    REQUIRE(destination.size() == sizeof(source));
    REQUIRE(destination[0] == 'J');
    REQUIRE(destination[2] == 'B');
    REQUIRE(destination[3] == '\0');
    REQUIRE(destination[4] == 'Y');
    REQUIRE(destination[7] == 'L');
}

TEST_CASE("YamlSink respects string_view length for owned string destination",
          "[job_yaml][sink]")
{
    constexpr char source[] = "Joseph-extra";
    const std::string_view value{source, 6};

    std::string destination;

    REQUIRE(YamlSink::scalar(destination, value));
    REQUIRE(destination == "Joseph");
}

// ========================================
// String view destination policy
// ========================================

TEST_CASE("YamlSink rejects string_view as scalar parse destination",
          "[job_yaml][sink]")
{
    std::string_view destination = "original";

    REQUIRE_FALSE(YamlSink::scalar(destination, "replacement"));
    REQUIRE(destination == "original");
}

// ========================================
// Empty scalar behavior
// ========================================

TEST_CASE("YamlSink rejects empty numeric scalar without changing destination",
          "[job_yaml][sink]")
{
    int destination = 42;

    REQUIRE_FALSE(YamlSink::scalar(destination, ""));
    REQUIRE(destination == 42);
}

TEST_CASE("YamlSink rejects empty boolean scalar without changing destination",
          "[job_yaml][sink]")
{
    bool destination = true;

    REQUIRE_FALSE(YamlSink::scalar(destination, ""));
    REQUIRE(destination);
}

TEST_CASE("YamlSink accepts empty scalar for owned string destination",
          "[job_yaml][sink]")
{
    std::string destination = "value";

    REQUIRE(YamlSink::scalar(destination, ""));
    REQUIRE(destination.empty());
}

// ========================================
// Whitespace ownership
// ========================================

TEST_CASE("YamlSink does not trim integer scalar input", "[job_yaml][sink]")
{
    int destination = 42;

    REQUIRE_FALSE(YamlSink::scalar(destination, " 7"));
    REQUIRE(destination == 42);

    REQUIRE_FALSE(YamlSink::scalar(destination, "7 "));
    REQUIRE(destination == 42);
}

TEST_CASE("YamlSink does not trim boolean scalar input", "[job_yaml][sink]")
{
    bool destination = true;

    REQUIRE_FALSE(YamlSink::scalar(destination, " false"));
    REQUIRE(destination);

    REQUIRE_FALSE(YamlSink::scalar(destination, "false "));
    REQUIRE(destination);
}

TEST_CASE("YamlSink does not trim floating point scalar input",
          "[job_yaml][sink]")
{
    double destination = 2.5;

    REQUIRE_FALSE(YamlSink::scalar(destination, " 3.5"));
    REQUIRE(destination == 2.5);

    REQUIRE_FALSE(YamlSink::scalar(destination, "3.5 "));
    REQUIRE(destination == 2.5);
}

// ========================================
// Non-null-terminated scalar input
// ========================================

TEST_CASE("YamlSink accepts non null terminated integer string_view",
          "[job_yaml][sink]")
{
    constexpr char source[] = {
        'x',
        '1', '2', '3',
        'x'
    };

    const std::string_view value{source + 1, 3};
    int destination{};

    REQUIRE(YamlSink::scalar(destination, value));
    REQUIRE(destination == 123);
}

// ========================================
// Reflected member destination
// ========================================

TEST_CASE("YamlSink reads reflected signed integer member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "count", "42"));
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlSink reads reflected unsigned integer member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "size", "1024"));
    REQUIRE(object.size == 1024U);
}

TEST_CASE("YamlSink reads reflected boolean member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "enabled", "true"));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlSink reads reflected float member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "scale", "1.5"));
    REQUIRE(object.scale == 1.5F);
}

TEST_CASE("YamlSink reads reflected double member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "ratio", "2.25"));
    REQUIRE(object.ratio == 2.25);
}

TEST_CASE("YamlSink reads reflected owned string member", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "name", "Joseph"));
    REQUIRE(object.name == "Joseph");
}

// ========================================
// Reflected member failures
// ========================================

TEST_CASE("YamlSink returns false for unknown reflected member",
          "[job_yaml][sink]")
{
    ObjectReaderObject object{
        .count = 7,
    };

    REQUIRE_FALSE(YamlSink::member(object, "missing", "42"));
    REQUIRE(object.count == 7);
}

TEST_CASE("YamlSink preserves reflected member on failed conversion",
          "[job_yaml][sink]")
{
    ObjectReaderObject object{
        .count = 17,
    };

    REQUIRE_FALSE(YamlSink::member(object, "count", "bad"));
    REQUIRE(object.count == 17);
}

TEST_CASE("YamlSink reflected member failure does not affect other members",
          "[job_yaml][sink]")
{
    ObjectReaderObject object{
        .count = 42,
        .size = 1024,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "Joseph",
    };

    REQUIRE_FALSE(YamlSink::member(object, "ratio", "bad"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "Joseph");
}

// ========================================
// Multiple member assignments
// ========================================

TEST_CASE("YamlSink populates multiple reflected members", "[job_yaml][sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlSink::member(object, "count", "42"));
    REQUIRE(YamlSink::member(object, "size", "1024"));
    REQUIRE(YamlSink::member(object, "enabled", "true"));
    REQUIRE(YamlSink::member(object, "scale", "1.5"));
    REQUIRE(YamlSink::member(object, "ratio", "2.25"));
    REQUIRE(YamlSink::member(object, "name", "Joseph"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "Joseph");
}

// ========================================
// Scalar and member semantic agreement
// ========================================

TEST_CASE("YamlSink scalar and member routes produce same integer result",
          "[job_yaml][sink]")
{
    int scalarDestination{};
    ObjectReaderObject object{};

    REQUIRE(YamlSink::scalar(scalarDestination, "-123"));
    REQUIRE(YamlSink::member(object, "count", "-123"));

    REQUIRE(scalarDestination == object.count);
}

TEST_CASE("YamlSink scalar and member routes produce same floating point result",
          "[job_yaml][sink]")
{
    double scalarDestination{};
    ObjectReaderObject object{};

    REQUIRE(YamlSink::scalar(scalarDestination, "3.125"));
    REQUIRE(YamlSink::member(object, "ratio", "3.125"));

    REQUIRE(scalarDestination == object.ratio);
}

TEST_CASE("YamlSink scalar and member routes produce same string result",
          "[job_yaml][sink]")
{
    std::string scalarDestination;
    ObjectReaderObject object{};

    REQUIRE(YamlSink::scalar(scalarDestination, "Joseph"));
    REQUIRE(YamlSink::member(object, "name", "Joseph"));

    REQUIRE(scalarDestination == object.name);
}

TEST_CASE("YamlSink accepts integer spelling for floating destinations",
          "[job_yaml][sink][float]")
{
    float f{};
    double d{};

    REQUIRE(YamlSink::scalar(f, "250"));
    REQUIRE(YamlSink::scalar(d, "250"));

    CHECK(f == 250.0f);
    CHECK(d == 250.0);
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlSink scalar benchmarks", "[job_yaml][sink][benchmark]")
{
    std::string_view integerValue = "42";
    std::string_view booleanValue = "true";
    std::string_view doubleValue = "3.1415926535";

    benchmarkDoNotOptimize(integerValue);
    benchmarkDoNotOptimize(booleanValue);
    benchmarkDoNotOptimize(doubleValue);

    BENCHMARK("YamlSink integer")
    {
        int destination{};

        benchmarkDoNotOptimize(integerValue);

        const bool result = YamlSink::scalar(destination, integerValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("direct ScalarKernel integer")
    {
        int destination{};

        benchmarkDoNotOptimize(integerValue);

        const bool result =
            YamlScalarKernel::parseInteger(integerValue, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("YamlSink boolean")
    {
        bool destination = false;

        benchmarkDoNotOptimize(booleanValue);

        const bool result = YamlSink::scalar(destination, booleanValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("direct ScalarKernel boolean")
    {
        bool destination = false;

        benchmarkDoNotOptimize(booleanValue);

        const bool result =
            YamlScalarKernel::parseBool(booleanValue, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("YamlSink double")
    {
        double destination{};

        benchmarkDoNotOptimize(doubleValue);

        const bool result = YamlSink::scalar(destination, doubleValue);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("direct ScalarKernel double")
    {
        double destination{};

        benchmarkDoNotOptimize(doubleValue);

        const bool result =
            YamlScalarKernel::parseFloat(doubleValue, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };
}

TEST_CASE("YamlSink owned string benchmark", "[job_yaml][sink][benchmark]")
{
    std::string_view value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlSink string")
    {
        std::string destination;

        benchmarkDoNotOptimize(value);

        const bool result = YamlSink::scalar(destination, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination.size();
    };

    BENCHMARK("direct string assign")
    {
        std::string destination;

        benchmarkDoNotOptimize(value);

        destination.assign(value);

        benchmarkClobber(destination);

        return destination.size();
    };
}

TEST_CASE("YamlSink reflected member benchmarks",
          "[job_yaml][sink][benchmark]")
{
    std::string_view countKey = "count";
    std::string_view countValue = "42";

    benchmarkDoNotOptimize(countKey);
    benchmarkDoNotOptimize(countValue);

    BENCHMARK("YamlSink reflected integer member")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(countKey);
        benchmarkDoNotOptimize(countValue);

        const bool result =
            YamlSink::member(object, countKey, countValue);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };

    BENCHMARK("direct YamlObjectReader integer member")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(countKey);
        benchmarkDoNotOptimize(countValue);

        const bool result =
            YamlObjectReader::readScalar(object, countKey, countValue);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlSink complete reflected object benchmark",
          "[job_yaml][sink][benchmark]")
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

    BENCHMARK("YamlSink six member object")
    {
        ObjectReaderObject object{};

        YamlSink::member(object, countKey, countValue);
        YamlSink::member(object, sizeKey, sizeValue);
        YamlSink::member(object, enabledKey, enabledValue);
        YamlSink::member(object, scaleKey, scaleValue);
        YamlSink::member(object, ratioKey, ratioValue);
        YamlSink::member(object, nameKey, nameValue);

        benchmarkClobber(object);

        return object.count +
               static_cast<int>(object.size) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.scale) +
               static_cast<int>(object.ratio) +
               static_cast<int>(object.name.size());
    };

    BENCHMARK("direct YamlObjectReader six member object")
    {
        ObjectReaderObject object{};

        bool ok = true;
        ok &= YamlObjectReader::readScalar(object, countKey, countValue);
        ok &= YamlObjectReader::readScalar(object, sizeKey, sizeValue);
        ok &= YamlObjectReader::readScalar(object, enabledKey, enabledValue);
        ok &= YamlObjectReader::readScalar(object, scaleKey, scaleValue);
        ok &= YamlObjectReader::readScalar(object, ratioKey, ratioValue);
        ok &= YamlObjectReader::readScalar(object, nameKey, nameValue);

        benchmarkDoNotOptimize(ok);
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