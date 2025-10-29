#include <catch2/catch_test_macros.hpp>
#include "async_event_loop.h"

using namespace job::threads;

TEST_CASE("AsyncEventLoop executes posted tasks", "[event_loop]") {
    AsyncEventLoop loop;
    loop.start();

    std::atomic<bool> ran{false};
    loop.post([&]{ ran = true; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop.stop();

    REQUIRE(ran.load());
}

TEST_CASE("AsyncEventLoop delayed execution", "[event_loop]") {
    AsyncEventLoop loop;
    loop.start();

    std::atomic<bool> ran{false};
    auto start = std::chrono::steady_clock::now();

    loop.postDelayed([&]{
        ran = true;
    }, std::chrono::milliseconds(200));

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    loop.stop();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    REQUIRE(ran.load());
    REQUIRE(elapsed.count() >= 200);
}

TEST_CASE("AsyncEventLoop repeating timer cancels correctly", "[event_loop]") {
    AsyncEventLoop loop;
    loop.start();

    std::atomic<int> counter{0};
    auto id = loop.addTimer([&]{ counter++; }, std::chrono::milliseconds(100), true);

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    loop.cancelTimer(id);
    auto afterCancel = counter.load();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    loop.stop();

     // stopped incrementing
    REQUIRE(afterCancel == counter.load());
}


TEST_CASE("AsyncEventLoop repeating timer fires multiple times", "[event_loop][timing]") {
    AsyncEventLoop loop;
    loop.start();

    std::atomic<int> ticks{0};
    loop.addTimer([&]{ ticks++; }, std::chrono::milliseconds(50), true);

    std::this_thread::sleep_for(std::chrono::milliseconds(260));
    loop.stop();

    REQUIRE(ticks.load() >= 4);
}


TEST_CASE("AsyncEventLoop cancel non-existent timer", "[event_loop][edge]") {
    AsyncEventLoop loop;
    loop.start();
    REQUIRE_FALSE(loop.cancelTimer(9999));
    loop.stop();
}
