#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("ThreadPool create and shutdown", "[threading][thread_pool]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 2);
    REQUIRE(pool != nullptr);

    pool->shutdown();
    REQUIRE(scheduler->size() == 0);
}

TEST_CASE("ThreadPool submit task and get future", "[threading][thread_pool]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 1);
    auto future = pool->submit([]() {
        return std::string("Hello");
    });

    REQUIRE(future.get() == "Hello");
    pool->shutdown();
}

TEST_CASE("ThreadPool run multiple tasks 4 threads", "[threading][thread_pool]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 4);
    std::atomic<int> counter = 0;

    for (int i = 0; i < 100; ++i) {
        pool->submit([&]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(1ms);
        });
    }

    pool->waitForIdle(1ms);
    REQUIRE(counter.load() == 100);

    pool->shutdown();
}

TEST_CASE("ThreadPool waitForIdle", "[threading][thread_pool]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 1);
    std::atomic<bool> task_running = false;
    std::atomic<bool> task_finished = false;

    pool->submit([&]() {
        task_running.store(true);
        std::this_thread::sleep_for(50ms);
        task_finished.store(true);
    });

    std::this_thread::sleep_for(10ms);
    REQUIRE(task_running.load() == true);
    REQUIRE(task_finished.load() == false);

    pool->waitForIdle(1ms);
    REQUIRE(task_finished.load() == true);

    pool->shutdown();
}

TEST_CASE("ThreadPool submit after shutdown", "[threading][thread_pool]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 1);

    auto future1 = pool->submit([]{ return 1; });
    REQUIRE(future1.get() == 1);

    pool->shutdown();

    auto future2 = pool->submit([]{ return 2; });
    auto future3 = pool->submit(1, []{ return 3; });
    REQUIRE_FALSE(future2.valid());
    REQUIRE_FALSE(future3.valid());
}

TEST_CASE("ThreadPool priority is strictly ordered", "[threading][thread_pool][priority]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 1);
    std::vector<int> execution_order;
    std::mutex vector_mutex;

    auto make_task = [&](int id) {
        return [&, id]() {
            std::this_thread::sleep_for(1ms);
            std::lock_guard<std::mutex> lock(vector_mutex);
            execution_order.push_back(id);
        };
    };

    pool->submit(10, make_task(1)); // Task 1 (prio 10)
    pool->submit(5,  make_task(2)); // Task 2 (prio 5)
    pool->submit(1,  make_task(3)); // Task 3 (prio 1)

    pool->waitForIdle(1ms);
    pool->shutdown();

    // A FifoScheduler executes in the order submitted (1, 2, 3)
    // A future PriorityScheduler would execute (3, 2, 1)
    REQUIRE(execution_order.size() == 3);
    REQUIRE(execution_order[0] == 1);
    REQUIRE(execution_order[1] == 2);
    REQUIRE(execution_order[2] == 3);
}

TEST_CASE("ThreadPool futures correctly throw exceptions", "[threading][thread_pool][exceptions]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 1);

    auto future = pool->submit([]() { throw std::runtime_error("BOOM!"); });
    REQUIRE_THROWS_AS(future.get(), std::runtime_error);

    // The pool should still be healthy
    auto future2 = pool->submit([]{ return 42; });
    REQUIRE(future2.get() == 42);

    pool->shutdown();
}

#ifndef JOB_CI_BUILD
TEST_CASE("ThreadPool submit() is thread-safe (concurrency)", "[threading][thread_pool][bench][concurrency]")
{
    job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
    INFO("Starting ThreadPool submit() stress test...");

    constexpr int kNumHammerThreads = 8;
    constexpr int kTasksPerThread = 200;
    std::atomic<int> counter{0};
    std::vector<std::thread> hammer_threads;

    auto scheduler = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(scheduler, 8);

    for (int i = 0; i < kNumHammerThreads; ++i) {
        hammer_threads.emplace_back([&]() {
            for (int j = 0; j < kTasksPerThread; ++j)
                pool->submit([&]{ counter.fetch_add(1, std::memory_order_relaxed); });
        });
    }

    for (auto& t : hammer_threads)
        t.join();

    INFO("All submit threads joined. Waiting for pool to idle...");
    pool->waitForIdle(1ms);
    pool->shutdown();

    REQUIRE(counter.load() == kNumHammerThreads * kTasksPerThread);
    SUCCEED("All tasks completed.");
}
#endif
