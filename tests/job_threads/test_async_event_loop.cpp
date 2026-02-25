#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

#include <job_async_event_loop.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("AsyncEventLoop post and stop", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_ran{false};

    loop.start();
    REQUIRE(loop.isRunning());

    loop.post([&] {
        task_ran.store(true);
    });

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(task_ran.load() == true);
    loop.stop();
    REQUIRE_FALSE(loop.isRunning());
}

TEST_CASE("AsyncEventLoop postDelayed", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_ran{false};
    auto start_time = std::chrono::steady_clock::now();

    loop.start();
    loop.postDelayed([&] {
        task_ran.store(true);
    }, 50ms);

    REQUIRE_FALSE(task_ran.load());
    std::this_thread::sleep_for(25ms);
    REQUIRE_FALSE(task_ran.load());

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    REQUIRE(task_ran.load() == true);
    REQUIRE(duration.count() >= 50);

    loop.stop();
}

TEST_CASE("AsyncEventLoop repeating timer", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<int> counter{0};

    loop.start();

    uint64_t timer_id = loop.addTimer([&] {
        counter++;
    }, 20ms, true);

    REQUIRE(timer_id > 0);

    std::this_thread::sleep_for(50ms);

    REQUIRE(counter.load() >= 2);
    REQUIRE(counter.load() <= 3);

    loop.stop();
}

TEST_CASE("AsyncEventLoop cancelTimer", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<int> counter{0};

    loop.start();

    uint64_t timer_id = loop.addTimer([&] {
        counter++;
    }, 20ms, true);

    std::this_thread::sleep_for(50ms);

    bool canceled = loop.cancelTimer(timer_id);
    REQUIRE(canceled == true);

    int count_after_cancel = counter.load();
    REQUIRE(count_after_cancel >= 2);

    std::this_thread::sleep_for(50ms);

    REQUIRE(counter.load() == count_after_cancel);

    bool canceled_again = loop.cancelTimer(timer_id);
    REQUIRE(canceled_again == false);

    loop.stop();
}

TEST_CASE("AsyncEventLoop globalLoop", "[threading][async_loop]")
{
    auto &loop = AsyncEventLoop::globalLoop();
    REQUIRE(loop.isRunning());

    std::atomic<bool> task_ran{false};
    loop.post([&] { task_ran.store(true); });

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(task_ran.load() == true);
    // We don't stop the global loop Because well ....... it's global
}

TEST_CASE("AsyncEventLoop handles re-entrancy (post from a timer)", "[threading][async_loop][reentrancy]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_from_timer_ran{false};
    loop.start();
    loop.postDelayed([&] {
        loop.post([&] {
            task_from_timer_ran.store(true);
        });
    }, 10ms);

    int retries = 0;
    // this needs to go somewhere else . . .
    while (!task_from_timer_ran.load() && retries < 100) {
        std::this_thread::sleep_for(2ms);
        retries++;
    }

    REQUIRE(task_from_timer_ran.load() == true);
    loop.stop();
}
