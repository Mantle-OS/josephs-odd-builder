#ifdef JOB_YAML_CPP_BENCH

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <yaml-cpp/yaml.h>

#include "test_job_yaml_utils.h"

#include <string>
#include <string_view>

namespace job::yaml::tests {

// ========================================
// yaml-cpp scalar benchmark
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("yaml-cpp node scalar benchmark",
          "[job_yaml][yaml_cpp][node][benchmark]")
{
    std::string value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp Node scalar assignment")
    {
        YAML::Node node;

        benchmarkDoNotOptimize(value);

        node = value;

        benchmarkClobber(node);

        return node.Scalar().size();
    };
}

// ========================================
// yaml-cpp mapping benchmark
// ========================================

TEST_CASE("yaml-cpp node mapping benchmark",
          "[job_yaml][yaml_cpp][node][benchmark]")
{
    std::string key = "name";
    std::string value = "Joseph";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp Node mapping assignment")
    {
        YAML::Node node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        node[key] = value;

        benchmarkClobber(node);

        return node.size();
    };
}

// ========================================
// yaml-cpp sequence benchmark
// ========================================

TEST_CASE("yaml-cpp node sequence benchmark",
          "[job_yaml][yaml_cpp][node][benchmark]")
{
    std::string value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp Node sequence push_back")
    {
        YAML::Node node;

        benchmarkDoNotOptimize(value);

        node.push_back(value);

        benchmarkClobber(node);

        return node.size();
    };
}

// ========================================
// yaml-cpp structural mapping benchmark
// ========================================

TEST_CASE("yaml-cpp structural mapping benchmark",
          "[job_yaml][yaml_cpp][node][v1][benchmark]")
{
    std::string key = "child";
    std::string value = "Joseph";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp structural setMember")
    {
        YAML::Node root;
        YAML::Node child;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        child = value;
        root[key] = child;

        benchmarkClobber(root);

        return root.size();
    };

    BENCHMARK("yaml-cpp scalar mapping assignment")
    {
        YAML::Node root;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        root[key] = value;

        benchmarkClobber(root);

        return root.size();
    };
}

// ========================================
// yaml-cpp structural sequence benchmark
// ========================================

TEST_CASE("yaml-cpp structural sequence benchmark",
          "[job_yaml][yaml_cpp][node][v1][benchmark]")
{
    std::string value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp structural append")
    {
        YAML::Node root;
        YAML::Node child;

        benchmarkDoNotOptimize(value);

        child = value;
        root.push_back(child);

        benchmarkClobber(root);

        return root.size();
    };

    BENCHMARK("yaml-cpp scalar push_back")
    {
        YAML::Node root;

        benchmarkDoNotOptimize(value);

        root.push_back(value);

        benchmarkClobber(root);

        return root.size();
    };
}

// ========================================
// yaml-cpp nested mapping benchmark
// ========================================

TEST_CASE("yaml-cpp nested mapping construction benchmark",
          "[job_yaml][yaml_cpp][node][v1][benchmark]")
{
    std::string value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp mapping containing mapping")
    {
        YAML::Node child;

        child["name"] = value;
        child["count"] = "42";

        YAML::Node root;
        root["child"] = child;

        benchmarkClobber(root);

        return root["child"].size();
    };
}

// ========================================
// yaml-cpp sequence of sequences benchmark
// ========================================

TEST_CASE("yaml-cpp sequence of sequences benchmark",
          "[job_yaml][yaml_cpp][node][v1][benchmark]")
{
    std::string key = "key";
    std::string value = "value";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("yaml-cpp sequence of two-value sequences")
    {
        YAML::Node entry;

        entry.push_back(key);
        entry.push_back(value);

        YAML::Node root;
        root.push_back(entry);

        benchmarkClobber(root);

        return root[0].size();
    };
}

#endif

} // namespace job::yaml::tests

#endif