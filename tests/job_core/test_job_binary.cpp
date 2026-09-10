#include <job_binary.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <chrono>
#include <iostream>
#include <string_view>
#endif

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "job_base_obj.h"

namespace job::binary::tests {

struct BinaryObject : job::core::BaseObject
{
    int count{};
    bool enabled{};
    std::uint64_t requestId{};
    std::string name{};
    std::vector<std::uint32_t> values{};
};

TEST_CASE("JobBinary serializes and deserializes scalar", "[job_binary]")
{
    constexpr std::uint32_t source = 0x12345678;

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));
    REQUIRE(bytes.size() == sizeof(source));

    std::uint32_t destination{};

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination == source);
}

TEST_CASE("JobBinary serializes and deserializes string", "[job_binary]")
{
    const std::string source = "JOB Binary";

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::string destination;

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination == source);
}

TEST_CASE("JobBinary serializes and deserializes optional", "[job_binary]")
{
    std::optional<int> source{42};

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::optional<int> destination;

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination.has_value());
    REQUIRE(*destination == 42);
}

TEST_CASE("JobBinary serializes empty optional", "[job_binary]")
{
    std::optional<int> source;

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::optional<int> destination{42};

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE_FALSE(destination.has_value());
}

TEST_CASE("JobBinary serializes and deserializes vector", "[job_binary]")
{
    const std::vector<int> source{1, 2, 3, 4, 5};

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::vector<int> destination;

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination == source);
}

TEST_CASE("JobBinary serializes and deserializes fixed array", "[job_binary]")
{
    const std::array<int, 4> source{10, 20, 30, 40};

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::array<int, 4> destination{};

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination == source);
}

TEST_CASE("JobBinary serializes and deserializes map", "[job_binary]")
{
    const std::unordered_map<std::string, int> source{
                                                      {"one", 1},
                                                      {"two", 2},
                                                      {"three", 3},
                                                      };

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    std::unordered_map<std::string, int> destination;

    REQUIRE(JobBinary::fromBytes(bytes, destination));
    REQUIRE(destination == source);
}

TEST_CASE("JobBinary serializes and deserializes reflected object", "[job_binary]")
{
    BinaryObject source;
    source.count = 42;
    source.enabled = true;
    source.requestId = 123456789;
    source.name = "JOB";
    source.values = {1, 2, 3, 4, 5};

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    BinaryObject destination;

    REQUIRE(JobBinary::fromBytes(bytes, destination));

    REQUIRE(destination.count == source.count);
    REQUIRE(destination.enabled == source.enabled);
    REQUIRE(destination.requestId == source.requestId);
    REQUIRE(destination.name == source.name);
    REQUIRE(destination.values == source.values);
}

TEST_CASE("JobBinary rejects trailing bytes", "[job_binary]")
{
    const int source = 42;

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));

    bytes.push_back(0xff);

    int destination{};

    REQUIRE_FALSE(JobBinary::fromBytes(bytes, destination));
}

TEST_CASE("JobBinary rejects truncated input", "[job_binary]")
{
    const std::uint64_t source = 42;

    std::vector<std::uint8_t> bytes;

    REQUIRE(JobBinary::toBytes(source, bytes));
    REQUIRE(bytes.size() > 1);

    bytes.pop_back();

    std::uint64_t destination{};

    REQUIRE_FALSE(JobBinary::fromBytes(bytes, destination));
}

#ifdef JOB_TEST_BENCHMARKS

struct BenchmarkResult
{
    double elapsedNs{};

    [[nodiscard]] double average(std::size_t iterations) const noexcept
    {
        return elapsedNs / static_cast<double>(iterations);
    }
};

template <typename Function>
BenchmarkResult runBenchmark(std::size_t warmupIterations,
                             std::size_t iterations,
                             Function &&function)
{
    for (std::size_t i = 0; i < warmupIterations; ++i)
        function();

    const auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < iterations; ++i)
        function();

    const auto end = std::chrono::steady_clock::now();

    return {
        .elapsedNs = std::chrono::duration<double, std::nano>(end - start).count(),
    };
}

void printBenchmark(std::string_view name,
                    std::size_t iterations,
                    const BenchmarkResult &result)
{
    std::cout << '\n' << name << '\n'
              << "  iterations: " << iterations << '\n'
              << "  total:      " << result.elapsedNs / 1'000'000.0 << " ms\n"
              << "  average:    " << result.average(iterations) << " ns/iteration\n";
}

TEST_CASE("JobBinary benchmark", "[job_binary][benchmark]")
{
    constexpr std::size_t Iterations = 1'000'000;
    constexpr std::size_t WarmupIterations = 100'000;

    BinaryObject source;
    source.count = 42;
    source.enabled = true;
    source.requestId = 123456789;
    source.name = "Joseph's Odd Builder";
    source.values = {1, 2, 3, 4, 5, 6, 7, 8};

    std::vector<std::uint8_t> buffer;

    REQUIRE(JobBinary::toBytes(source, buffer));

    {
        std::vector<std::uint8_t> output;

        const auto result = runBenchmark(WarmupIterations, Iterations, [&] {
            if (!JobBinary::toBytes(source, output))
                FAIL("JobBinary encode failed");

            asm volatile("" : : "g"(output.data()), "g"(output.size()) : "memory");
        });

        printBenchmark("JobBinary encode benchmark", Iterations, result);
        std::cout << "  bytes:      " << output.size() << '\n';
    }

    {
        BinaryObject destination;

        const auto result = runBenchmark(WarmupIterations, Iterations, [&] {
            if (!JobBinary::fromBytes(buffer, destination))
                FAIL("JobBinary decode failed");

            asm volatile("" : : "g"(destination.requestId), "g"(destination.values.size()) : "memory");
        });

        printBenchmark("JobBinary decode benchmark", Iterations, result);
    }

    {
        constexpr std::size_t RoundTripIterations = 500'000;

        std::vector<std::uint8_t> output;
        BinaryObject destination;

        const auto result = runBenchmark(WarmupIterations, RoundTripIterations, [&] {
            if (!JobBinary::toBytes(source, output))
                FAIL("JobBinary encode failed");

            if (!JobBinary::fromBytes(output, destination))
                FAIL("JobBinary decode failed");

            asm volatile("" : : "g"(destination.requestId), "g"(destination.values.size()) : "memory");
        });

        printBenchmark("JobBinary round-trip benchmark", RoundTripIterations, result);
    }
}

#ifndef JOB_CI_BUILD

TEST_CASE("JobBinary large object benchmark", "[job_binary][benchmark][heavy]")
{
    constexpr std::size_t Iterations = 100'000;
    constexpr std::size_t WarmupIterations = 1'000;

    BinaryObject source;
    source.count = 42;
    source.enabled = true;
    source.requestId = 123456789;
    source.name = "JOB Binary large object";
    source.values.resize(4096);

    for (std::size_t i = 0; i < source.values.size(); ++i)
        source.values[i] = static_cast<std::uint32_t>(i);

    std::vector<std::uint8_t> buffer;
    BinaryObject destination;

    const auto result = runBenchmark(WarmupIterations, Iterations, [&] {
        if (!JobBinary::toBytes(source, buffer))
            FAIL("JobBinary encode failed");

        if (!JobBinary::fromBytes(buffer, destination))
            FAIL("JobBinary decode failed");

        asm volatile("" : : "g"(destination.values.data()), "g"(destination.values.size()) : "memory");
    });

    printBenchmark("JobBinary large round-trip benchmark", Iterations, result);
    std::cout << "  payload:    " << buffer.size() << " bytes\n";
}

#endif // NOT JOB_CI_BUILD

#endif // JOB_TEST_BENCHMARKS

} // namespace job::binary::tests