#include <catch2/catch_test_macros.hpp>
#include <thread_pool.h>

using namespace job::threads;

TEST_CASE("ThreadPool executes submitted jobs", "[thread_pool]") {
    auto pool = ThreadPool::create(4);
    std::atomic<int> value{0};

    auto f1 = pool->submit([&]{ value.fetch_add(1); });
    auto f2 = pool->submit([&]{ value.fetch_add(1); });

    f1.get();
    f2.get();

    pool->shutdown();
    REQUIRE(value.load() == 2);
}

TEST_CASE("ThreadPool range tracking", "[thread_pool]") {
    auto pool = ThreadPool::create(2);
    pool->setTaskRange(0, 10);

    for (int i = 0; i < 5; ++i)
        pool->submit([]{});

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pool->taskMax() == 10);
    pool->shutdown();
}

TEST_CASE("ThreadPool handles exceptions safely", "[thread_pool]") {
    auto pool = ThreadPool::create(2);
    std::atomic<bool> ran{false};

    auto f = pool->submit([&]{
        ran = true;
        throw std::runtime_error("boom");
    });

    f.wait();
    REQUIRE(ran);
    pool->shutdown();
}


TEST_CASE("ThreadPool runs many small tasks", "[thread_pool][stress]") {
    auto pool = ThreadPool::create(8);
    std::atomic<int> counter{0};

    for (int i = 0; i < 1000; ++i)
        pool->submit([&]{ counter.fetch_add(1, std::memory_order_relaxed); });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    pool->shutdown();

    REQUIRE(counter.load() == 1000);
}

TEST_CASE("ThreadPool shutdown stops idle workers", "[thread_pool][shutdown]") {
    auto pool = ThreadPool::create(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool->shutdown();
    REQUIRE(pool->taskCount() == 0);
}
