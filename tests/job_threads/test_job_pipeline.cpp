#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>
#include <utils/job_pipeline.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobPipeline connects stages linearly", "[threading][pipeline][linear]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 2);

    JobPipeline pipe(pool);

    auto input = pipe.createInput<int>();
    auto to_string_stage = pipe.createStage<int, std::string>([](int &&val) {
        return std::to_string(val);
    });

    // sink
    std::atomic<bool> received{false};
    std::string result_val;

    auto sink = pipe.createSink<std::string>([&](std::string &&val) {
        result_val = val;
        received.store(true);
    });

    // connector
    JobPipeline::connect(input, to_string_stage, sink);
    REQUIRE(input->post(42));

    // this again . . . . I will fix this soon lol
    int retries = 0;
    while (!received.load() && retries < 100) {
        std::this_thread::sleep_for(5ms);
        retries++;
    }

    REQUIRE(received.load() == true);
    REQUIRE(result_val == "42");

    // 'pipe' dies, they die.
    pool->shutdown();
}

TEST_CASE("JobPipeline handles Fan-Out", "[threading][pipeline][fanout]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);

    JobPipeline pipe(pool);

    auto source = pipe.createInput<int>();

    std::atomic<int> sum{0};

    auto sinkA = pipe.createSink<int>([&](int &&v){
        sum.fetch_add(v);
    });

    auto sinkB = pipe.createSink<int>([&](int &&v){
        sum.fetch_add(v);
    });

    // Fan-Out ? try at least
    JobPipeline::connectFanOut(source, std::vector{sinkA, sinkB});

    REQUIRE(source->post(10));

    // 10 + 10 = 20
    int retries = 0;
    while (sum.load() < 20 && retries < 100) {
        std::this_thread::sleep_for(5ms);
        retries++;
    }

    REQUIRE(sum.load() == 20);
    pool->shutdown();
}

TEST_CASE("JobPipeline stage can filter messages with std::nullopt", "[threading][pipeline][filter]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);
    JobPipeline pipe(pool);

    auto input = pipe.createInput<int>();

    // drop odd numbers
    auto evens = pipe.createStage<int, int>(
        [](int &&v) -> std::optional<int> {
            if (v % 2 == 0)
                return v;
            return std::nullopt;
        });

    std::atomic<int> sum{0};
    auto sink = pipe.createSink<int>([&](int &&v) {
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    JobPipeline::connect(input, evens, sink);

    REQUIRE(input->post(1));
    REQUIRE(input->post(2));
    REQUIRE(input->post(3));
    REQUIRE(input->post(4));

    int retries = 0;
    while (sum.load(std::memory_order_relaxed) < 6 && retries < 100) {
        std::this_thread::sleep_for(5ms);
        ++retries;
    }

    REQUIRE(sum.load() == 6); // 2 + 4
    pool->shutdown();
}

TEST_CASE("JobPipeline stage invokes error handler on exception", "[threading][pipeline][errors]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 2);
    JobPipeline pipe(pool);

    auto input = pipe.createInput<std::string>();

    auto parse = pipe.createStage<std::string, int>(
        [](std::string &&s) -> std::optional<int> {
            if (s == "boom")
                throw std::runtime_error("parse error");
            return std::stoi(s);
        });

    std::atomic<int> errorCount{0};
    std::string lastBad;

    parse->setErrorHandler([&](const std::exception& e, const std::string& msg){
        (void)e;
        errorCount.fetch_add(1, std::memory_order_relaxed);
        lastBad = msg; // should see the *original* "boom"
    });

    std::atomic<int> sum{0};
    auto sink = pipe.createSink<int>([&](int &&v){
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    JobPipeline::connect(input, parse, sink);

    REQUIRE(input->post("10"));
    REQUIRE(input->post("boom"));  // throws
    REQUIRE(input->post("32"));

    int retries = 0;
    while (sum.load() < 42 && retries < 200) {
        std::this_thread::sleep_for(5ms);
        ++retries;
    }

    REQUIRE(sum.load() == 42);
    REQUIRE(errorCount.load() == 1);
    REQUIRE(lastBad == "boom");

    // And the metrics on the stage should reflect both messages processed,
    // even with one throwing.
    REQUIRE(parse->getProcessedCount() == 2);
    REQUIRE(parse->getErrorCount() == 1);

    pool->shutdown();
}

TEST_CASE("JobPipeline processes many messages end-to-end", "[threading][pipeline][load]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);
    JobPipeline pipe(pool);

    auto input  = pipe.createInput<int>();
    auto square = pipe.createStage<int, int>(
        [](int &&v) -> std::optional<int> {
            return v * v;
        });

    std::atomic<long long> sum{0};
    auto sink = pipe.createSink<int>([&](int &&v){
        sum.fetch_add(v, std::memory_order_relaxed);
    });

    JobPipeline::connect(input, square, sink);

    constexpr int N = 1000;
    for (int i = 0; i < N; ++i)
        REQUIRE(input->post(i));

    // sum of squares: 0^2 + ... + (N-1)^2 = (N-1)N(2N-1)/6
    long long expected = static_cast<long long>(N - 1) * N * (2LL * N - 1) / 6;

    int retries = 0;
    while (sum.load() != expected && retries < 500) {
        std::this_thread::sleep_for(5ms);
        ++retries;
    }

    REQUIRE(sum.load() == expected);

    // might as well and check the metrics
    REQUIRE(square->getProcessedCount() == static_cast<size_t>(N));
    REQUIRE(square->getErrorCount() == 0);

    pool->shutdown();
}
