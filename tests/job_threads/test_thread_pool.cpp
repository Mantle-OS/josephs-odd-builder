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
