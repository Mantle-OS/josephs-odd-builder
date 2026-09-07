#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include "test_job_yaml_utils.h"
#endif

#include <array>
#include <string>
#include <string_view>

#include <yaml-cpp/yaml.h>

#include "test_yaml_cpp_fixtures.h"

namespace job::yaml::tests {

// ========================================
// yaml-cpp reflected mapping equivalent
// ========================================

TEST_CASE("yaml-cpp parses scalar mapping",
          "[job_yaml][yaml_cpp][parser]")
{
    // yaml-cpp's public YAML::Load API accepts std::string, const char *,
    // or std::istream. Use std::string intentionally so this comparison
    // follows yaml-cpp's supported API rather than adapting JOB's
    // std::string_view input.
    const std::string source =
        "name: Joseph\n"
        "count: 42\n"
        "enabled: true\n"
        "ratio: 1.5\n";

    const YAML::Node node = YAML::Load(source);

    YamlCppParserDestinationFixture object;
    object.name = node["name"].as<std::string>();
    object.count = node["count"].as<int>();
    object.enabled = node["enabled"].as<bool>();
    object.ratio = node["ratio"].as<double>();

    REQUIRE(object.name == "Joseph");
    REQUIRE(object.count == 42);
    REQUIRE(object.enabled);
    REQUIRE(object.ratio == 1.5);
}

TEST_CASE("yaml-cpp parser comparison uses supported std::string input",
          "[job_yaml][yaml_cpp][parser]")
{
    const std::string source = "count: 42\n";

    const YAML::Node node = YAML::Load(source);

    REQUIRE(node["count"].as<int>() == 42);
}

// ========================================
// Scalar grammar comparison
// ========================================

TEST_CASE("yaml-cpp scalar grammar comparison cases",
          "[job_yaml][yaml_cpp][parser][scalar]")
{
    struct ScalarCase
    {
        std::string_view source;
        std::string_view expected;
    };

    constexpr std::array cases{
        ScalarCase{"alpha#beta", "alpha#beta"},
        ScalarCase{"alpha # comment", "alpha"},
        ScalarCase{"'it''s'", "it's"},
        ScalarCase{"\"hello\\nworld\"", "hello\nworld"},
        ScalarCase{"\"\\u0041\"", "A"},
        ScalarCase{"\"\\U0001F642\"", "\xF0\x9F\x99\x82"}
    };

    for (const ScalarCase &testCase : cases) {
        const YAML::Node node = YAML::Load(std::string{testCase.source});
        const std::string value = node.as<std::string>();

        REQUIRE(value == testCase.expected);
    }
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("yaml-cpp reflected mapping equivalent benchmark",
          "[job_yaml][yaml_cpp][parser][benchmark]")
{
    const std::string source =
        "name: Joseph\n"
        "count: 42\n"
        "enabled: true\n"
        "ratio: 1.5\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp reflected mapping equivalent")
    {
        YAML::Node node = YAML::Load(source);

        YamlCppParserDestinationFixture object;
        object.name = node["name"].as<std::string>();
        object.count = node["count"].as<int>();
        object.enabled = node["enabled"].as<bool>();
        object.ratio = node["ratio"].as<double>();

        benchmarkClobber(object);
        benchmarkClobber(node);

        return object.count +
               static_cast<int>(object.name.size()) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.ratio);
    };
}

TEST_CASE("yaml-cpp single scalar mapping benchmark",
          "[job_yaml][yaml_cpp][parser][benchmark]")
{
    const std::string source = "count: 42\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp single reflected scalar equivalent")
    {
        YAML::Node node = YAML::Load(source);

        YamlCppParserDestinationFixture object;
        object.count = node["count"].as<int>();

        benchmarkClobber(object);
        benchmarkClobber(node);

        return object.count;
    };
}

TEST_CASE("yaml-cpp root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][benchmark]")
{
    const std::string source = "Joseph";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

TEST_CASE("yaml-cpp single quoted root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][scalar][benchmark]")
{
    const std::string source = "'Joseph'";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp single quoted root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

TEST_CASE("yaml-cpp double quoted root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][scalar][benchmark]")
{
    const std::string source = "\"Joseph\"";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp double quoted root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

TEST_CASE("yaml-cpp transformed single quoted root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][scalar][benchmark]")
{
    const std::string source = "'it''s fine'";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp transformed single quoted root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

TEST_CASE("yaml-cpp transformed double quoted root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][scalar][benchmark]")
{
    const std::string source = "\"hello\\nworld\"";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp transformed double quoted root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

TEST_CASE("yaml-cpp plain hash boundary root scalar benchmark",
          "[job_yaml][yaml_cpp][parser][scalar][benchmark]")
{
    const std::string source = "alpha#beta";

    benchmarkDoNotOptimize(source);

    BENCHMARK("yaml-cpp plain hash boundary root scalar")
    {
        YAML::Node node = YAML::Load(source);
        std::string value = node.as<std::string>();

        benchmarkClobber(node);
        benchmarkClobber(value);

        return value.size();
    };
}

#endif

} // namespace job::yaml::tests