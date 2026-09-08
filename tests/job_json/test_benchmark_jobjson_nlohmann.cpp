#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#include <job_json.h>
#include <nlohmann/json.hpp>

#include "test_job_json_utils.h"
#include "test_nlohmann_fixtures.h"


namespace job::json::tests {

TEST_CASE("JobJson and nlohmann benchmark correctness gate", "[job_json][benchmark][nlohmann]")
{
    const auto expected = makeNlohmannBenchmarkFixture();

    JsonRoundTripFixture jobParsed;

    REQUIRE(JobJson::fromJson(NlohmannBenchmarkJson, jobParsed));
    REQUIRE(benchmarkFixturesEqual(jobParsed, expected));

    const auto nlohmannJson =
        nlohmann::json::parse(NlohmannBenchmarkJson);

    const auto nlohmannParsed =
        nlohmannJson.get<JsonRoundTripFixture>();

    REQUIRE(benchmarkFixturesEqual(nlohmannParsed, expected));

    std::string jobOutput;

    REQUIRE(JobJson::toJson(expected, jobOutput));

    const auto nlohmannOutput =
        nlohmann::json(expected).dump();

    JsonRoundTripFixture jobOutputParsed;
    REQUIRE(JobJson::fromJson(jobOutput, jobOutputParsed));
    REQUIRE(benchmarkFixturesEqual(jobOutputParsed, expected));

    const auto nlohmannOutputParsed =
        nlohmann::json::parse(nlohmannOutput)
            .get<JsonRoundTripFixture>();

    REQUIRE(benchmarkFixturesEqual(nlohmannOutputParsed, expected));
}

TEST_CASE("Benchmark JobJson versus nlohmann parse", "[job_json][benchmark][nlohmann]")
{
    constexpr std::size_t Iterations = 1'000'000;

    JsonRoundTripFixture jobValue;

    const double jobNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                JobJson::fromJson(
                    NlohmannBenchmarkJson,
                    jobValue);

                benchmarkDoNotOptimize(jobValue);
            });

    JsonRoundTripFixture nlohmannValue;

    const double nlohmannNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                const auto json =
                    nlohmann::json::parse(
                        NlohmannBenchmarkJson);

                nlohmannValue =
                    json.get<JsonRoundTripFixture>();

                benchmarkDoNotOptimize(nlohmannValue);
            });

    std::cout
        << "\nJSON parse benchmark\n"
        << "  iterations: " << Iterations << '\n'
        << "  JobJson:    " << jobNanoseconds << " ns/iteration\n"
        << "  nlohmann:   " << nlohmannNanoseconds << " ns/iteration\n"
        << "  ratio:      " << nlohmannNanoseconds / jobNanoseconds << "x\n";

    REQUIRE(jobNanoseconds > 0.0);
    REQUIRE(nlohmannNanoseconds > 0.0);
}

TEST_CASE("Benchmark JobJson versus nlohmann emit", "[job_json][benchmark][nlohmann]")
{
    constexpr std::size_t Iterations = 1'000'000;

    const auto value = makeNlohmannBenchmarkFixture();

    std::string jobOutput;
    jobOutput.reserve(NlohmannBenchmarkJson.size());

    const double jobNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                JobJson::toJson(value, jobOutput);
                benchmarkDoNotOptimize(jobOutput);
            });

    std::string nlohmannOutput;

    const double nlohmannNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                nlohmannOutput =
                    nlohmann::json(value).dump();

                benchmarkDoNotOptimize(nlohmannOutput);
            });

    std::cout
        << "\nJSON emit benchmark\n"
        << "  iterations: " << Iterations << '\n'
        << "  JobJson:    " << jobNanoseconds << " ns/iteration\n"
        << "  nlohmann:   " << nlohmannNanoseconds << " ns/iteration\n"
        << "  ratio:      " << nlohmannNanoseconds / jobNanoseconds << "x\n";

    REQUIRE(jobNanoseconds > 0.0);
    REQUIRE(nlohmannNanoseconds > 0.0);
}

TEST_CASE("Benchmark JobJson versus nlohmann round trip", "[job_json][benchmark][nlohmann]")
{
    constexpr std::size_t Iterations = 500'000;

    JsonRoundTripFixture jobValue;
    std::string jobOutput;
    jobOutput.reserve(NlohmannBenchmarkJson.size());

    const double jobNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                JobJson::fromJson(
                    NlohmannBenchmarkJson,
                    jobValue);

                JobJson::toJson(
                    jobValue,
                    jobOutput);

                benchmarkDoNotOptimize(jobValue);
                benchmarkDoNotOptimize(jobOutput);
            });

    JsonRoundTripFixture nlohmannValue;
    std::string nlohmannOutput;

    const double nlohmannNanoseconds =
        benchmarkNanosecondsPerIteration(
            Iterations,
            [&] {
                const auto json =
                    nlohmann::json::parse(
                        NlohmannBenchmarkJson);

                nlohmannValue =
                    json.get<JsonRoundTripFixture>();

                nlohmannOutput =
                    nlohmann::json(nlohmannValue).dump();

                benchmarkDoNotOptimize(nlohmannValue);
                benchmarkDoNotOptimize(nlohmannOutput);
            });

    std::cout
        << "\nJSON round-trip benchmark\n"
        << "  iterations: " << Iterations << '\n'
        << "  JobJson:    " << jobNanoseconds << " ns/iteration\n"
        << "  nlohmann:   " << nlohmannNanoseconds << " ns/iteration\n"
        << "  ratio:      " << nlohmannNanoseconds / jobNanoseconds << "x\n";

    REQUIRE(jobNanoseconds > 0.0);
    REQUIRE(nlohmannNanoseconds > 0.0);
}

} // namespace job::json::tests

