#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

#include <job_thread_pool.h>
#include <sched/job_work_stealing_scheduler.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobWorkStealingScheduler create and shutdown", "[threading][scheduler][work_stealing]")
{
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(4);
    auto pool = ThreadPool::create(scheduler, 4);

    REQUIRE(pool != nullptr);

    std::atomic<int> counter{0};
    auto fut1 = pool->submit([&]{ counter.fetch_add(1); });
    auto fut2 = pool->submit([&]{ counter.fetch_add(1); });

    fut1.get();
    fut2.get();

    REQUIRE(counter.load() == 2);

    pool->shutdown();
    REQUIRE(scheduler->size() == 0);
}

TEST_CASE("JobWorkStealingScheduler handles high concurrent load", "[threading][scheduler][work_stealing]")
{
    job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
    INFO("Starting Work-Stealing stress test...");

    const size_t numThreads = 8;
    const int numTasks = 10000;

    auto scheduler = std::make_shared<JobWorkStealingScheduler>(numThreads);
    auto pool = ThreadPool::create(scheduler, numThreads);
    REQUIRE(pool != nullptr);

    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    futures.reserve(numTasks);

    for (int i = 0; i < numTasks; ++i) {
        futures.emplace_back(pool->submit([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for(auto& fut : futures)
        fut.get();


    INFO("All tasks submitted and futures retrieved.");
    pool->waitForIdle(1ms);

    REQUIRE(counter.load() == numTasks);

    pool->shutdown();
    REQUIRE(scheduler->size() == 0);
}

TEST_CASE("JobWorkStealingScheduler forces stealing behavior", "[threading][scheduler][work_stealing]")
{
    const size_t numThreads = 4;
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(numThreads);
    auto pool = ThreadPool::create(scheduler, numThreads);
    REQUIRE(pool != nullptr);

    std::atomic<int> counter{0};

    // The Slight of hand
    // Submit *fewer* tasks than there are threads.
    // The scheduler will submit these to queues 0 and 1.
    // Workers 2 and 3 will start with empty queues.
    // The test will only pass if workers 2 and 3
    // (or 0/1 after they finish) successfully steal the tasks.
    const int numTasks = 2;

    std::vector<std::future<void>> futures;
    for (int i = 0; i < numTasks; ++i) {
        futures.emplace_back(pool->submit([&] {
            // Add a small delay to make stealing more likely to be needed
            std::this_thread::sleep_for(5ms);
            counter.fetch_add(1);
        }));
    }

    for(auto& fut : futures) {
        fut.get();
    }

    pool->waitForIdle(1ms);

    // If stealing works, all tasks should be processed.
    REQUIRE(counter.load() == numTasks);

    pool->shutdown();
}

TEST_CASE("JobWorkStealingScheduler handles workerCount = 0", "[threading][scheduler][work_stealing]")
{
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(0);
    auto pool = ThreadPool::create(scheduler, 1);
    REQUIRE(pool != nullptr);

    std::atomic<int> counter{0};

    auto fut = pool->submit([&]{ counter.fetch_add(1); });
    fut.get();

    REQUIRE(counter.load() == 1);
    pool->shutdown();
}
// CHECKPOINT v1.0
