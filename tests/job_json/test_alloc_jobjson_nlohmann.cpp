#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

#include <job_json.h>
#include <nlohmann/json.hpp>

#include "test_job_json_utils.h"
#include "test_nlohmann_fixtures.h"


    namespace job::json::tests {

    struct AllocationStats
    {
        std::size_t allocations{};
        std::size_t deallocations{};
        std::size_t bytes{};
    };

    inline thread_local bool allocationTrackingEnabled = false;
    inline thread_local AllocationStats allocationStats{};

    inline void recordAllocation(std::size_t size) noexcept
    {
        if (!allocationTrackingEnabled)
            return;

        ++allocationStats.allocations;
        allocationStats.bytes += size;
    }

    inline void recordDeallocation() noexcept
    {
        if (allocationTrackingEnabled)
            ++allocationStats.deallocations;
    }

    inline void beginAllocationTracking() noexcept
    {
        allocationStats = {};
        allocationTrackingEnabled = true;
    }

    [[nodiscard]] inline AllocationStats endAllocationTracking() noexcept
    {
        allocationTrackingEnabled = false;
        return allocationStats;
    }

    [[nodiscard]] inline void *allocateTracked(std::size_t size)
    {
        if (size == 0)
            size = 1;

        void *memory = std::malloc(size);

        if (!memory)
            throw std::bad_alloc{};

        recordAllocation(size);
        return memory;
    }

    [[nodiscard]] inline void *allocateTrackedAligned(std::size_t size, std::size_t alignment)
    {
        if (size == 0)
            size = 1;

        void *memory = nullptr;

        if (::posix_memalign(&memory, alignment, size) != 0)
            throw std::bad_alloc{};

        recordAllocation(size);
        return memory;
    }

    inline void deallocateTracked(void *memory) noexcept
    {
        if (!memory)
            return;

        recordDeallocation();
        std::free(memory);
    }

    inline void printAllocationResult(
        std::string_view name,
        const AllocationStats &stats,
        std::size_t iterations)
    {
        std::cout << "  " << name << ":\n"
                  << "    allocations:    " << stats.allocations << '\n'
                  << "    allocations/op: " << static_cast<double>(stats.allocations) / static_cast<double>(iterations) << '\n'
                  << "    bytes:          " << stats.bytes << '\n'
                  << "    bytes/op:       " << static_cast<double>(stats.bytes) / static_cast<double>(iterations) << '\n';
    }

} // namespace job::json::tests


void *operator new(std::size_t size)
{
    return job::json::tests::allocateTracked(size);
}

void *operator new[](std::size_t size)
{
    return job::json::tests::allocateTracked(size);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
    try {
        return job::json::tests::allocateTracked(size);
    } catch (...) {
        return nullptr;
    }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
    try {
        return job::json::tests::allocateTracked(size);
    } catch (...) {
        return nullptr;
    }
}

void *operator new(std::size_t size, std::align_val_t alignment)
{
    return job::json::tests::allocateTrackedAligned(size, static_cast<std::size_t>(alignment));
}

void *operator new[](std::size_t size, std::align_val_t alignment)
{
    return job::json::tests::allocateTrackedAligned(size, static_cast<std::size_t>(alignment));
}

void *operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept
{
    try {
        return job::json::tests::allocateTrackedAligned(size, static_cast<std::size_t>(alignment));
    } catch (...) {
        return nullptr;
    }
}

void *operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept
{
    try {
        return job::json::tests::allocateTrackedAligned(size, static_cast<std::size_t>(alignment));
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void *memory) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete(void *memory, std::size_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory, std::size_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete(void *memory, const std::nothrow_t &) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory, const std::nothrow_t &) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete(void *memory, std::align_val_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory, std::align_val_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete(void *memory, std::size_t, std::align_val_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory, std::size_t, std::align_val_t) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete(void *memory, std::align_val_t, const std::nothrow_t &) noexcept
{
    job::json::tests::deallocateTracked(memory);
}

void operator delete[](void *memory, std::align_val_t, const std::nothrow_t &) noexcept
{
    job::json::tests::deallocateTracked(memory);
}


namespace job::json::tests {

TEST_CASE("Allocation benchmark JobJson versus nlohmann parse", "[job_json][allocation][nlohmann]")
{
    constexpr std::size_t Iterations = 10'000;

    bool jobSuccess = true;

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        JsonRoundTripFixture value;
        jobSuccess = JobJson::fromJson(NlohmannBenchmarkJson, value) && jobSuccess;
        benchmarkDoNotOptimize(value);
    }

    const AllocationStats jobStats = endAllocationTracking();

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        const auto json = nlohmann::json::parse(NlohmannBenchmarkJson);
        const auto value = json.get<JsonRoundTripFixture>();
        benchmarkDoNotOptimize(value);
    }

    const AllocationStats nlohmannStats = endAllocationTracking();

    REQUIRE(jobSuccess);

    std::cout << "\nJSON parse allocation benchmark\n"
              << "  iterations: " << Iterations << '\n';

    printAllocationResult("JobJson", jobStats, Iterations);
    printAllocationResult("nlohmann", nlohmannStats, Iterations);
}

TEST_CASE("Allocation benchmark JobJson versus nlohmann emit", "[job_json][allocation][nlohmann]")
{
    constexpr std::size_t Iterations = 10'000;

    const auto value = makeNlohmannBenchmarkFixture();

    bool jobSuccess = true;

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        std::string output;
        jobSuccess = JobJson::toJson(value, output) && jobSuccess;
        benchmarkDoNotOptimize(output);
    }

    const AllocationStats jobStats = endAllocationTracking();

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        const std::string output = nlohmann::json(value).dump();
        benchmarkDoNotOptimize(output);
    }

    const AllocationStats nlohmannStats = endAllocationTracking();

    REQUIRE(jobSuccess);

    std::cout << "\nJSON emit allocation benchmark\n"
              << "  iterations: " << Iterations << '\n';

    printAllocationResult("JobJson", jobStats, Iterations);
    printAllocationResult("nlohmann", nlohmannStats, Iterations);
}

TEST_CASE("Allocation benchmark JobJson versus nlohmann round trip", "[job_json][allocation][nlohmann]")
{
    constexpr std::size_t Iterations = 10'000;

    bool jobSuccess = true;

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        JsonRoundTripFixture value;
        std::string output;

        jobSuccess = JobJson::fromJson(NlohmannBenchmarkJson, value) && jobSuccess;
        jobSuccess = JobJson::toJson(value, output) && jobSuccess;

        benchmarkDoNotOptimize(value);
        benchmarkDoNotOptimize(output);
    }

    const AllocationStats jobStats = endAllocationTracking();

    beginAllocationTracking();

    for (std::size_t iteration = 0; iteration < Iterations; ++iteration) {
        const auto json = nlohmann::json::parse(NlohmannBenchmarkJson);
        const auto value = json.get<JsonRoundTripFixture>();
        const std::string output = nlohmann::json(value).dump();

        benchmarkDoNotOptimize(value);
        benchmarkDoNotOptimize(output);
    }

    const AllocationStats nlohmannStats = endAllocationTracking();

    REQUIRE(jobSuccess);

    std::cout << "\nJSON round-trip allocation benchmark\n"
              << "  iterations: " << Iterations << '\n';

    printAllocationResult("JobJson", jobStats, Iterations);
    printAllocationResult("nlohmann", nlohmannStats, Iterations);
}

} // namespace job::json::tests

