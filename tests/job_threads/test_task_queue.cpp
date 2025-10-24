#include <catch2/catch_test_macros.hpp>
#include "task_queue.h"
#include <thread>

using namespace job::threads;

TEST_CASE("TaskQueue basic post/take", "[task_queue]") {
    TaskQueue queue;
    bool executed = false;

    queue.post([&]{ executed = true; });
    auto fn = queue.take();
    fn();

    REQUIRE(executed);
}

TEST_CASE("TaskQueue priority ordering", "[task_queue]") {
    TaskQueue queue;
    std::vector<int> result;

    queue.post([&]{ result.push_back(1); }, 5);
    queue.post([&]{ result.push_back(2); }, 1); // higher priority

    auto fn1 = queue.take();
    fn1();
    auto fn2 = queue.take();
    fn2();

    REQUIRE(result == std::vector<int>({2, 1}));
}

TEST_CASE("TaskQueue blocking take", "[task_queue]") {
    TaskQueue queue;
    std::atomic<bool> flag{false};

    std::thread t([&] {
        auto fn = queue.take();
        fn();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    queue.post([&]{ flag = true; });
    t.join();

    REQUIRE(flag.load());
}
