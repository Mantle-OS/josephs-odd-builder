#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_object_sink.h>
#include <job_yaml_sink.h>

#include "../tests-fast-math-workaround.h"
#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

#include <string_view>

namespace job::yaml::tests {

// ========================================
// Signed integer member
// ========================================

TEST_CASE("YamlObjectSink writes signed integer member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "count", "42"));
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectSink overwrites signed integer member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .count = 7,
    };

    REQUIRE(YamlObjectSink::member(object, "count", "-42"));
    REQUIRE(object.count == -42);
}

// ========================================
// Unsigned integer member
// ========================================

TEST_CASE("YamlObjectSink writes unsigned integer member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "size", "1024"));
    REQUIRE(object.size == 1024U);
}

TEST_CASE("YamlObjectSink writes hexadecimal unsigned integer member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "size", "0xFF"));
    REQUIRE(object.size == 255U);
}

// ========================================
// Boolean member
// ========================================

TEST_CASE("YamlObjectSink writes boolean member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "enabled", "true"));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectSink overwrites boolean member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .enabled = true,
    };

    REQUIRE(YamlObjectSink::member(object, "enabled", "false"));
    REQUIRE_FALSE(object.enabled);
}

// ========================================
// Floating point members
// ========================================

TEST_CASE("YamlObjectSink writes float member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "scale", "1.5"));
    REQUIRE(object.scale == 1.5F);
}

TEST_CASE("YamlObjectSink writes double member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "ratio", "2.25"));
    REQUIRE(object.ratio == 2.25);
}

TEST_CASE("YamlObjectSink writes special floating point members",
          "[job_yaml][object_sink][fast_math]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "scale", ".inf"));
    REQUIRE(isSafePositiveInfinity(object.scale));

    REQUIRE(YamlObjectSink::member(object, "ratio", "-.inf"));
    REQUIRE(isSafeNegativeInfinity(object.ratio));
}

// ========================================
// String member
// ========================================

TEST_CASE("YamlObjectSink writes owned string member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "name", "Joseph"));
    REQUIRE(object.name == "Joseph");
}

TEST_CASE("YamlObjectSink preserves exact string payload",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "name", "  Joseph  "));
    REQUIRE(object.name == "  Joseph  ");
}

TEST_CASE("YamlObjectSink accepts empty string payload",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .name = "old",
    };

    REQUIRE(YamlObjectSink::member(object, "name", ""));
    REQUIRE(object.name.empty());
}

TEST_CASE("YamlObjectSink does not interpret string syntax",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "name", "\"Joseph\\n\""));
    REQUIRE(object.name == "\"Joseph\\n\"");
}

// ========================================
// Unknown members
// ========================================

TEST_CASE("YamlObjectSink returns false for unknown member",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectSink::member(object, "missing", "42"));
}

TEST_CASE("YamlObjectSink unknown member leaves object unchanged",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .count = 7,
        .size = 11,
        .enabled = true,
        .scale = 1.25F,
        .ratio = 2.5,
        .name = "Joseph",
    };

    REQUIRE_FALSE(YamlObjectSink::member(object, "missing", "999"));

    REQUIRE(object.count == 7);
    REQUIRE(object.size == 11U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.25F);
    REQUIRE(object.ratio == 2.5);
    REQUIRE(object.name == "Joseph");
}

TEST_CASE("YamlObjectSink returns false for empty member key",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectSink::member(object, "", "42"));
}

// ========================================
// Failed conversions
// ========================================

TEST_CASE("YamlObjectSink preserves integer member after failed conversion",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .count = 17,
    };

    REQUIRE_FALSE(YamlObjectSink::member(object, "count", "bad"));
    REQUIRE(object.count == 17);
}

TEST_CASE("YamlObjectSink preserves boolean member after failed conversion",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .enabled = true,
    };

    REQUIRE_FALSE(YamlObjectSink::member(object, "enabled", "yes"));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectSink preserves floating member after failed conversion",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .ratio = 2.5,
    };

    REQUIRE_FALSE(YamlObjectSink::member(object, "ratio", "1.2.3"));
    REQUIRE(object.ratio == 2.5);
}

TEST_CASE("YamlObjectSink failed conversion leaves other members unchanged",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{
        .count = 42,
        .size = 1024,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "Joseph",
    };

    REQUIRE_FALSE(YamlObjectSink::member(object, "count", "bad"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "Joseph");
}

// ========================================
// Repeated assignments
// ========================================

TEST_CASE("YamlObjectSink supports repeated member assignment",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "count", "1"));
    REQUIRE(object.count == 1);

    REQUIRE(YamlObjectSink::member(object, "count", "2"));
    REQUIRE(object.count == 2);

    REQUIRE(YamlObjectSink::member(object, "count", "-3"));
    REQUIRE(object.count == -3);
}

TEST_CASE("YamlObjectSink remains usable after failed assignment",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectSink::member(object, "count", "bad"));
    REQUIRE(object.count == 0);

    REQUIRE(YamlObjectSink::member(object, "count", "42"));
    REQUIRE(object.count == 42);
}

// ========================================
// Multiple members
// ========================================

TEST_CASE("YamlObjectSink populates complete scalar object",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "count", "42"));
    REQUIRE(YamlObjectSink::member(object, "size", "1024"));
    REQUIRE(YamlObjectSink::member(object, "enabled", "true"));
    REQUIRE(YamlObjectSink::member(object, "scale", "1.5"));
    REQUIRE(YamlObjectSink::member(object, "ratio", "2.25"));
    REQUIRE(YamlObjectSink::member(object, "name", "Joseph"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "Joseph");
}

// ========================================
// string_view boundaries
// ========================================

TEST_CASE("YamlObjectSink accepts non null terminated key string_view",
          "[job_yaml][object_sink]")
{
    constexpr char source[] = {
        'x',
        'c', 'o', 'u', 'n', 't',
        'x'
    };

    const std::string_view key{source + 1, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, key, "73"));
    REQUIRE(object.count == 73);
}

TEST_CASE("YamlObjectSink accepts non null terminated value string_view",
          "[job_yaml][object_sink]")
{
    constexpr char source[] = {
        'x',
        '1', '2', '3',
        'x'
    };

    const std::string_view value{source + 1, 3};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "count", value));
    REQUIRE(object.count == 123);
}

TEST_CASE("YamlObjectSink preserves embedded null in string member",
          "[job_yaml][object_sink]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectSink::member(object, "name", value));
    REQUIRE(object.name.size() == sizeof(source));
    REQUIRE(object.name[3] == '\0');
    REQUIRE(object.name[7] == 'L');
}

// ========================================
// Facade agreement
// ========================================

TEST_CASE("YamlObjectSink and YamlSink produce same member result",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject objectSink{};
    ObjectReaderObject genericSink{};

    REQUIRE(YamlObjectSink::member(objectSink, "count", "-123"));
    REQUIRE(YamlSink::member(genericSink, "count", "-123"));

    REQUIRE(objectSink.count == genericSink.count);
}

TEST_CASE("YamlObjectSink and YamlObjectReader produce same member result",
          "[job_yaml][object_sink]")
{
    ObjectReaderObject sinkObject{};
    ObjectReaderObject readerObject{};

    REQUIRE(YamlObjectSink::member(sinkObject, "ratio", "3.125"));
    REQUIRE(YamlObjectReader::readScalar(readerObject, "ratio", "3.125"));

    REQUIRE(sinkObject.ratio == readerObject.ratio);
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlObjectSink reflected member benchmark",
          "[job_yaml][object_sink][benchmark]")
{
    std::string_view key = "count";
    std::string_view value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

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

    BENCHMARK("YamlSink integer member")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlSink::member(object, key, value);

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

TEST_CASE("YamlObjectSink complete object benchmark",
          "[job_yaml][object_sink][benchmark]")
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

    BENCHMARK("YamlObjectReader six member object")
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