#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include <job_thread_pool.h>
#include <sched/job_round_robin_scheduler.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobRoundRobinScheduler create and shutdown", "[threading][scheduler][round_robin]")
{
    auto scheduler = std::make_shared<JobRoundRobinScheduler>(4);
    auto pool = ThreadPool::create(scheduler, 2);

    REQUIRE(pool != nullptr);

    std::atomic<int> counter{0};

    auto fut1 = pool->submit([&]{ counter++; });
    auto fut2 = pool->submit([&]{ counter++; });

    fut1.get();
    fut2.get();

    REQUIRE(counter.load() == 2);

    pool->shutdown();
    REQUIRE(scheduler->size() == 0);
}

TEST_CASE("JobRoundRobinScheduler strictly alternates task priorities", "[threading][scheduler][round_robin]")
{
    auto scheduler = std::make_shared<JobRoundRobinScheduler>(2);
    auto pool = ThreadPool::create(scheduler, 1);
    REQUIRE(pool != nullptr);

    std::vector<std::string> execution_order;
    std::mutex vector_mutex;

    // Helper lambda to create a task
    auto make_task = [&](const std::string& id) {
        return [&, id]() {
            // Short sleep to ensure tasks don't finish before the next is scheduled
            std::this_thread::sleep_for(1ms);
            std::lock_guard<std::mutex> lock(vector_mutex);
            execution_order.push_back(id);
        };
    };

    pool->submit(0, make_task("P0-TaskA"));
    pool->submit(0, make_task("P0-TaskB"));
    pool->submit(0, make_task("P0-TaskC"));

    pool->submit(1, make_task("P1-TaskA"));
    pool->submit(1, make_task("P1-TaskB"));
    pool->submit(1, make_task("P1-TaskC"));

    pool->waitForIdle(2ms);
    pool->shutdown();

    REQUIRE(execution_order.size() == 6);
    REQUIRE(execution_order[0] == "P0-TaskA");
    REQUIRE(execution_order[1] == "P1-TaskA");
    REQUIRE(execution_order[2] == "P0-TaskB");
    REQUIRE(execution_order[3] == "P1-TaskB");
    REQUIRE(execution_order[4] == "P0-TaskC");
    REQUIRE(execution_order[5] == "P1-TaskC");
}

TEST_CASE("JobRoundRobinScheduler handles empty priority queues", "[threading][scheduler][round_robin]")
{
    auto scheduler = std::make_shared<JobRoundRobinScheduler>(2);
    auto pool = ThreadPool::create(scheduler, 1);
    REQUIRE(pool != nullptr);

    std::vector<std::string> execution_order;
    std::mutex vector_mutex;

    auto make_task = [&](const std::string& id) {
        return [&, id]() {
            std::this_thread::sleep_for(1ms);
            std::lock_guard<std::mutex> lock(vector_mutex);
            execution_order.push_back(id);
        };
    };

    pool->submit(0, make_task("P0-TaskA"));
    pool->submit(2, make_task("P2-TaskA"));
    pool->submit(0, make_task("P0-TaskB"));
    pool->submit(2, make_task("P2-TaskB"));

    pool->waitForIdle(2ms);
    pool->shutdown();

    REQUIRE(execution_order.size() == 4);
    REQUIRE(execution_order[0] == "P0-TaskA");
    REQUIRE(execution_order[1] == "P2-TaskA");
    REQUIRE(execution_order[2] == "P0-TaskB");
    REQUIRE(execution_order[3] == "P2-TaskB");
}
// CHECKPOINT v1.0
