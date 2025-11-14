#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <vector>
#include <numeric>
#include <limits>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <job_parallel_for.h>
#include <job_parallel_reduce.h>
#include "sched/job_fifo_scheduler.h"

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("parallel_for correctly processes all elements", "[threading][algorithms][parallel_for]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 8);
    REQUIRE(pool != nullptr);

    constexpr size_t kVectorSize = 100000;
    std::vector<std::atomic<int>> data(kVectorSize);
    for(size_t i = 0; i < kVectorSize; ++i)
        data[i].store(0);

    parallel_for(*pool, (size_t)0, kVectorSize, [&](size_t i) {
        data[i].fetch_add(1, std::memory_order_relaxed);
    });

    bool all_processed = true;
    for(size_t i = 0; i < kVectorSize; ++i) {
        if (data[i].load() != 1) {
            all_processed = false;
            break;
        }
    }

    REQUIRE(all_processed == true);

    std::atomic<bool> empty_ran = false;
    parallel_for(*pool, (size_t)0, (size_t)0, [&](size_t) {
        empty_ran.store(true);
    });
    REQUIRE(empty_ran.load() == false);

    pool->shutdown();
}

TEST_CASE("parallel_reduce correctly sums elements", "[threading][algorithms][parallel_reduce]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 8);
    REQUIRE(pool != nullptr);

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
    long long result = parallel_reduce(*pool, data.begin(), data.end(), init, map_fn, reduce_fn);

    REQUIRE(result == expected_sum);

    init = 1000;
    result = parallel_reduce(*pool, data.begin(), data.end(), init, map_fn, reduce_fn);
    REQUIRE(result == expected_sum + 1000);

    pool->shutdown();
}

TEST_CASE("parallel_reduce finds max element", "[threading][algorithms][parallel_reduce]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 8);
    REQUIRE(pool != nullptr);

    std::vector<int> data = {1, 5, 2, 100, 50, 99, 1000, 3, 45, 999};

    auto map_fn = [](int val) {
        return val;
    };

    auto reduce_fn = [](int a, int b) {
        return std::max(a, b);
    };

    int init = std::numeric_limits<int>::min();
    int max_val = parallel_reduce(*pool, data.begin(), data.end(), init, map_fn, reduce_fn);

    REQUIRE(max_val == 1000);

    std::vector<int> empty_data;
    max_val = parallel_reduce(*pool, empty_data.begin(), empty_data.end(), init, map_fn, reduce_fn);
    REQUIRE(max_val == init);

    pool->shutdown();
}
// VERSION: v1.0
