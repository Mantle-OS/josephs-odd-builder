#include <atomic>
#include <thread>
#include <vector>

#include <catch2/catch_all.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_stealing_ctx.h>
#include <simd_for.h>
#include <simd_provider.h>

#include "test_job_simd_utils.h"

using namespace job::simd;
using namespace job::simd::test;
using namespace job::threads;

// 1
TEST_CASE("simd_for splits work into a vector stride and a scalar tail", "[simd][for]")
{
    constexpr std::size_t size = 19; // deliberately not a multiple of SIMD::width()
    const std::size_t width = SIMD::width();

    std::vector<std::size_t> vectorIndices;
    std::vector<std::size_t> scalarIndices;

    simd_for(size,
             [&](std::size_t i) { vectorIndices.push_back(i); },
             [&](std::size_t i) { scalarIndices.push_back(i); });

    for (std::size_t i = 0; i < vectorIndices.size(); ++i)
        REQUIRE(vectorIndices[i] == i * width);

    const std::size_t tailStart = (size / width) * width;
    for (std::size_t i = 0; i < scalarIndices.size(); ++i)
        REQUIRE(scalarIndices[i] == tailStart + i);

    REQUIRE(vectorIndices.size() * width + scalarIndices.size() == size);
}

TEST_CASE("simd_for(size, ...) matches simd_for(0, size, ...)", "[simd][for]")
{
    constexpr std::size_t size = 37;
    std::vector<std::size_t> a, b;

    simd_for(size, [&](std::size_t i) { a.push_back(i); }, [&](std::size_t i) { a.push_back(i); });
    simd_for(std::size_t{0}, size, [&](std::size_t i) { b.push_back(i); }, [&](std::size_t i) { b.push_back(i); });

    REQUIRE(a == b);
}

TEST_CASE("parallel simd_for produces the same output as the serial version", "[simd][for][threads]")
{
    constexpr std::size_t size = 4096; // above kMinGrain so it actually parallelizes
    std::vector<float> serialOut(size);
    std::vector<float> parallelOut(size);

    auto vStep = [](std::size_t i, std::vector<float> &out) {
        SIMD::mov(&out[i], SIMD::mul_plus(SIMD::set1(static_cast<float>(i)), SIMD::set1(2.0f), SIMD::set1(1.0f)));
    };
    auto sStep = [](std::size_t i, std::vector<float> &out) {
        out[i] = static_cast<float>(i) * 2.0f + 1.0f;
    };

    simd_for(size,
             [&](std::size_t i) { vStep(i, serialOut); },
             [&](std::size_t i) { sStep(i, serialOut); });

    job::threads::JobStealerCtx ctx(job::simd::test::threadCountForTests());
    simd_for(*ctx.pool, size,
             [&](std::size_t i) { vStep(i, parallelOut); },
             [&](std::size_t i) { sStep(i, parallelOut); });

    REQUIRE(serialOut == parallelOut);
}

// 2
TEST_CASE("simd_for does nothing when start >= end", "[simd][for][edge]")
{
    bool called = false;
    simd_for(std::size_t{10}, std::size_t{5}, [&](std::size_t) { called = true; }, [&](std::size_t) { called = true; });
    REQUIRE_FALSE(called);
}

TEST_CASE("parallel simd_for falls back to the calling thread below the min grain", "[simd][for][edge][threads]")
{
    // Small enough to be under kMinGrain (1024) regardless of SIMD width,
    // so the pool overload should short-circuit to the serial path and
    // never actually hand work to a worker.
    constexpr std::size_t size = 10;
    const std::thread::id callingThread = std::this_thread::get_id();
    std::atomic<bool> ranOnOtherThread{false};

    JobStealerCtx ctx(threadCountForTests());
    simd_for(*ctx.pool, size,
             [&](std::size_t) {
                 if (std::this_thread::get_id() != callingThread)
                     ranOnOtherThread = true;
             },
             [&](std::size_t) {
                 if (std::this_thread::get_id() != callingThread)
                     ranOnOtherThread = true;
             });

    REQUIRE_FALSE(ranOnOtherThread);
}

TEST_CASE("parallel simd_for called from inside a worker task runs serially instead of deadlocking", "[simd][for][edge][threads]")
{
    // Exercises ThreadPool::inWorkerThread()'s reentrancy guard in
    // job::threads::parallel_for. If that guard is missing or broken,
    // this test hangs instead of failing cleanly.
    constexpr std::size_t outerSize = 4096;
    constexpr std::size_t innerSize = 4096;

    JobStealerCtx ctx(threadCountForTests());
    std::atomic<std::size_t> innerCallCount{0};

    simd_for(*ctx.pool, outerSize,
             [&](std::size_t) {
                 simd_for(*ctx.pool, innerSize,
                          [&](std::size_t) { innerCallCount.fetch_add(1, std::memory_order_relaxed); },
                          [&](std::size_t) { innerCallCount.fetch_add(1, std::memory_order_relaxed); });
             },
             [&](std::size_t) {
                 simd_for(*ctx.pool, innerSize,
                          [&](std::size_t) { innerCallCount.fetch_add(1, std::memory_order_relaxed); },
                          [&](std::size_t) { innerCallCount.fetch_add(1, std::memory_order_relaxed); });
             });

    REQUIRE(innerCallCount.load() > 0);
}


// 3
#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark: serial simd_for vs plain for (exp)", "[simd][for][benchmark]")
{
    constexpr std::size_t n      = 4'000'000;
    std::vector<float> data(n);
    for (std::size_t i = 0; i < n; ++i)
        data[i] = static_cast<float>(i % 20) - 10.0f;
    std::vector<float> out(n);

    BENCHMARK("for (serial, std::exp)")
    {
        for (std::size_t i = 0; i < n; ++i)
            out[i] = std::exp(data[i]);
        return out[0];
    };

    BENCHMARK("simd_for (serial)")
    {
        const float* srcData = data.data();
        float* dstData       = out.data();
        simd_for(n,
                 [&](std::size_t i) { SIMD::mov(dstData + i, SIMD::exp(SIMD::pull(srcData + i))); },
                 [&](std::size_t i) {
                     alignas(sizeof(float) * SIMD::width()) float tail[SIMD::width()] = {1.0f};
                     const std::size_t remaining = n - i;
                     std::memcpy(tail, srcData + i, remaining * sizeof(float));
                     SIMD::mov(tail, SIMD::exp(SIMD::pull(tail)));
                     std::memcpy(dstData + i, tail, remaining * sizeof(float));
                 });
        return out[0];
    };
}

TEST_CASE("Benchmark: serial simd_for vs parallel simd_for (bigger load)", "[simd][for][benchmark]")
{
    constexpr std::size_t nBig = 64'000'000;
    std::vector<float> dataBig(nBig);
    for (std::size_t i = 0; i < nBig; ++i)
        dataBig[i] = static_cast<float>(i % 20) - 10.0f;
    std::vector<float> outBig(nBig);

    JobStealerCtx ctx(threadCountForTests());

    BENCHMARK("simd_for (Big serial)")
    {
        const float* srcData = dataBig.data();
        float* dstData       = outBig.data();
        simd_for(nBig,
                 [&](std::size_t i) { SIMD::mov(dstData + i, SIMD::exp(SIMD::pull(srcData + i))); },
                 [&](std::size_t i) {
                     alignas(sizeof(float) * SIMD::width()) float tail[SIMD::width()] = {1.0f};
                     const std::size_t remaining = nBig - i;
                     std::memcpy(tail, srcData + i, remaining * sizeof(float));
                     SIMD::mov(tail, SIMD::exp(SIMD::pull(tail)));
                     std::memcpy(dstData + i, tail, remaining * sizeof(float));
                 });
        return outBig[0];
    };

    BENCHMARK("simd_for (Big parallel)")
    {
        const float* srcData = dataBig.data();
        float* dstData       = outBig.data();
        simd_for(*ctx.pool, nBig,
                 [&](std::size_t i) { SIMD::mov(dstData + i, SIMD::exp(SIMD::pull(srcData + i))); },
                 [&](std::size_t i) {
                     alignas(sizeof(float) * SIMD::width()) float tail[SIMD::width()] = {1.0f};
                     const std::size_t remaining = nBig - i;
                     std::memcpy(tail, srcData + i, remaining * sizeof(float));
                     SIMD::mov(tail, SIMD::exp(SIMD::pull(tail)));
                     std::memcpy(dstData + i, tail, remaining * sizeof(float));
                 });
        return outBig[0];
    };
}
#endif
