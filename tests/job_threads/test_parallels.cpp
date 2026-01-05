#include <catch2/catch_test_macros.hpp>
#include <unordered_set>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <atomic>
#include <vector>
#include <numeric>
#include <limits>
#include <thread>

#include <job_logger.h>

#include <ctx/job_fifo_ctx.h>

#include <job_parallel_for.h>
#include <job_parallel_reduce.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("parallel_for correctly processes all elements", "[threading][algorithms][parallel_for]") {
    JobFifoCtx sched(8);
    constexpr size_t kVectorSize = 100000;
    std::vector<std::atomic<int>> data(kVectorSize);
    for (size_t i = 0; i < kVectorSize; ++i) data[i].store(0);

    parallel_for(*sched.pool, size_t{0}, kVectorSize, [&](size_t i) {
        data[i].fetch_add(1, std::memory_order_relaxed);
    });

    for (size_t i = 0; i < kVectorSize; ++i) {
        REQUIRE(data[i].load(std::memory_order_relaxed) == 1);
    }

    std::atomic<bool> empty_ran = false;
    parallel_for(*sched.pool, size_t{0}, size_t{0}, [&](size_t) {
        empty_ran.store(true, std::memory_order_relaxed);
    });
    REQUIRE(empty_ran.load(std::memory_order_relaxed) == false);
}

TEST_CASE("parallel_for Distribution sanity (Linear): each worker must get ≥1 chunk", "[threading][parallel_for][distribution]") {
    const size_t workers = 8;
    JobFifoCtx sched(workers);

    const size_t N = 1u << 18;      // 262,144
    const size_t grain = 256;       // many chunks
    std::atomic<size_t> counter{0};

    std::mutex mtx;
    std::unordered_set<std::thread::id> threads_seen;

    parallel_for(*sched.pool, size_t{0}, N,
                 [&](size_t) {
                     // record which thread executed at least one iteration
                     static thread_local bool recorded = false;
                     if (!recorded) { // sample sparsely to reduce contention
                         std::lock_guard<std::mutex> lk(mtx);
                         threads_seen.insert(std::this_thread::get_id());
                         recorded = true;
                     }
                     counter.fetch_add(1, std::memory_order_relaxed);
                 },
                 0,
                 grain,
                 AccessPattern::Linear);

    REQUIRE(counter.load(std::memory_order_relaxed) == N);
    // We expect every worker thread to appear here.
    REQUIRE(threads_seen.size() == sched.pool->workerCount());
}

TEST_CASE("parallel_for Small-N cutoff: should run serially on the calling thread", "[threading][parallel_for][smallN]") {
    JobFifoCtx sched(8);

    // N smaller than default min_grain and explicit small grain
    for (size_t N : {0ul, 1ul, 7ul, 63ul}) {
        std::unordered_set<std::thread::id> threads_seen;
        std::mutex mtx;

        std::thread::id main_id = std::this_thread::get_id();
        parallel_for(*sched.pool, size_t{0}, N,
                     [&](size_t) {
                         std::lock_guard<std::mutex> lk(mtx);
                         threads_seen.insert(std::this_thread::get_id());
                     },
                     0,
                     64,                // ensure cutoff
                     AccessPattern::Linear
                     );

        // Only the calling thread should have executed.
        REQUIRE(threads_seen.size() <= 1);
        if (!threads_seen.empty()) {
            REQUIRE(*threads_seen.begin() == main_id);
        }
    }
}

TEST_CASE("parallel_for Nested call from within a worker recursion guard", "[threading][parallel_for][nested]") {
    JobFifoCtx sched(8);

    std::atomic<size_t> outer_sum{0};
    std::atomic<size_t> inner_sum{0};

    const size_t N_outer = 1024;
    const size_t N_inner = 257; // intentionally odd

    parallel_for(*sched.pool, size_t{0}, N_outer,
                 [&](size_t i) {
                     outer_sum.fetch_add(i, std::memory_order_relaxed);

                     // Nested call: should run inline on worker thread
                     parallel_for(*sched.pool, size_t{0}, N_inner,
                                  [&](size_t j) {
                                      inner_sum.fetch_add(j, std::memory_order_relaxed);
                                  });
                 },
                 0,
                 128,
                 AccessPattern::Linear);

    const size_t expected_outer = (N_outer - 1) * N_outer / 2;
    const size_t expected_inner = (N_inner - 1) * N_inner / 2 * N_outer;

    REQUIRE(outer_sum.load(std::memory_order_relaxed) == expected_outer);
    REQUIRE(inner_sum.load(std::memory_order_relaxed) == expected_inner);
}

TEST_CASE("parallel_for Exception path: we rethrow after joining all tasks", "[threading][parallel_for][exceptions]") {
    JobFifoCtx sched(8);

    const size_t N = 4096;
    std::atomic<size_t> progressed{0};

    struct Boom { };

    bool threw = false;
    try {
        parallel_for(*sched.pool, size_t{0}, N,
                     [&](size_t i) {
                         // Make one specific index throw
                         if (i == 1337)
                             throw Boom{};
                         progressed.fetch_add(1, std::memory_order_relaxed);
                     },
                     0,
                     64,
                     AccessPattern::Linear);
    } catch (const Boom&) {
        threw = true;
    } catch (...) {
        // Should not be a different type
        threw = false;
    }
    REQUIRE(threw == true);

    // can't guarantee how far others progressed, but we CAN assert that
    // some work beyond the throwing index was executed (join-all semantics).
    REQUIRE(progressed.load(std::memory_order_relaxed) >= 1337);
}

TEST_CASE("parallel_for Strided vs Linear: correctness + thread utilization sanity", "[threading][parallel_for][strided]") {
    JobFifoCtx sched(8);
    const size_t N = 1u << 18;
    const size_t grain = 128;

    std::vector<std::atomic<int>> a(N), b(N);
    for (size_t i = 0; i < N; ++i) {
        a[i].store(0);
        b[i].store(0);
    }

    // Linear
    {
        std::mutex m;
        std::unordered_set<std::thread::id> seen;
        parallel_for(*sched.pool, size_t{0}, N,
                     [&](size_t i) {
                         static thread_local bool recorded = false;
                         if (!recorded) {
                             std::lock_guard<std::mutex> lk(m);
                             seen.insert(std::this_thread::get_id());
                             recorded = true;
                         }
                         a[i].fetch_add(1, std::memory_order_relaxed);
                     },
                     0, grain, AccessPattern::Linear);
        REQUIRE(seen.size() == sched.pool->workerCount());
    }
    // Strided
    {
        std::mutex m;
        std::unordered_set<std::thread::id> seen;
        parallel_for(*sched.pool, size_t{0}, N,
                     [&](size_t i) {
                         static thread_local bool recorded = false;
                         if (!recorded) {
                             std::lock_guard<std::mutex> lk(m);
                             seen.insert(std::this_thread::get_id());
                              recorded = true;
                         }
                         b[i].fetch_add(1, std::memory_order_relaxed);
                     },
                     0, grain, AccessPattern::Strided);
        REQUIRE(seen.size() == sched.pool->workerCount());
    }
    // Both must have touched every element exactly once
    for (size_t i = 0; i < N; ++i) {
        REQUIRE(a[i].load(std::memory_order_relaxed) == 1);
        REQUIRE(b[i].load(std::memory_order_relaxed) == 1);
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("parallel_for benchmark Linear vs Strided on skewed workload", "[threading][algorithms][parallel_for][bench]") {
    JobFifoCtx sched(8);

    const size_t size = 1u << 20;
    const size_t grain = 128;

    auto skewed_work = [](size_t i) {
        if ((i % 256) == 0) {
            uint32_t x = static_cast<uint32_t>(i);
            for (int k = 0; k < 3000; ++k)
                x ^= x * 1664525u + 1013904223u;

            __asm__ volatile("" :: "r"(x));
        }
    };

    BENCHMARK("Linear skewed") {
        parallel_for(*sched.pool, size_t{0}, size, skewed_work, 0, grain, AccessPattern::Linear);
    };
    BENCHMARK("Strided skewed") {
        parallel_for(*sched.pool, size_t{0}, size, skewed_work, 0, grain, AccessPattern::Strided);
    };
}
#endif

//////////////////////////////////////////////////////////
/// end parallel_for


//////////////////////////////////////////////////////////
/// start parallel_reduce

TEST_CASE("parallel_reduce correctly sums elements", "[threading][algorithms][parallel_reduce]")
{
    JobFifoCtx sched(8);
    constexpr size_t kVectorSize = 100000;
    std::vector<int> data(kVectorSize);
    std::iota(data.begin(), data.end(), 1);

    // Calculate expected sum (n * (n + 1) / 2)
    long long expected_sum = (long long)kVectorSize * (kVectorSize + 1) / 2;

    auto map_fn = [](int val) -> long long {
        return static_cast<long long>(val);
    };

    auto reduce_fn = [](long long a, long long b) -> long long {
        return a + b;
    };

    long long init = 0;
    long long result = parallel_reduce(*sched.pool, data.begin(), data.end(), init, map_fn, reduce_fn);

    REQUIRE(result == expected_sum);

    init = 1000;
    result = parallel_reduce(*sched.pool, data.begin(), data.end(), init, map_fn, reduce_fn);
    REQUIRE(result == expected_sum + 1000);
}

TEST_CASE("parallel_reduce finds max element", "[threading][algorithms][parallel_reduce]")
{
    JobFifoCtx sched(8);
    std::vector<int> data = {1, 5, 2, 100, 50, 99, 1000, 3, 45, 999};

    auto map_fn = [](int val) {
        return val;
    };

    auto reduce_fn = [](int a, int b) {
        return std::max(a, b);
    };

    int init = std::numeric_limits<int>::min();
    int max_val = parallel_reduce(*sched.pool, data.begin(), data.end(), init, map_fn, reduce_fn);

    REQUIRE(max_val == 1000);

    std::vector<int> empty_data;
    max_val = parallel_reduce(*sched.pool, empty_data.begin(), empty_data.end(), init, map_fn, reduce_fn);
    REQUIRE(max_val == init);
}

#ifdef JOB_TEST_BENCHMARKS
#endif


