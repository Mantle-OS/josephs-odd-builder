#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>

#include <thread>
#include <vector>

#include <job_thread.h>
#include <job_logger.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobThread lifecycle with setRunFunction (Composition)", "[threading][lifecycle]")
{
    std::atomic<bool> didRun{false};
    std::atomic<bool> stopTokenWasHonored{false};
    std::atomic<int>  loopCount{0};

    JobThread thread;
    thread.setRunFunction([&](std::stop_token token) {
        didRun.store(true);
        while (!token.stop_requested()) {
            loopCount++;
            std::this_thread::sleep_for(1ms);
        }
        stopTokenWasHonored.store(true);
    });

    REQUIRE_FALSE(thread.isRunning());
    REQUIRE_FALSE(didRun.load());

    REQUIRE(thread.start() == JobThread::StartResult::Started);
    REQUIRE(thread.isRunning());

    std::this_thread::sleep_for(10ms);

    REQUIRE(didRun.load() == true);
    REQUIRE(loopCount.load() > 0);

    REQUIRE(thread.start() == JobThread::StartResult::AlreadyRunning);

    thread.requestStop();

    REQUIRE(thread.join() == true);

    REQUIRE_FALSE(thread.isRunning());
    REQUIRE(stopTokenWasHonored.load() == true);

    REQUIRE(thread.join() == false);
}

TEST_CASE("JobThread lifecycle with overridden run (Inheritance)", "[threading][lifecycle]")
{
    class MyTestThread : public JobThread {
    public:
        std::atomic<bool> didRun{false};
        std::atomic<bool> stopTokenWasHonored{false};
        MyTestThread() : JobThread(JobThreadOptions::normal()) {}
    protected:
        void run(std::stop_token token) noexcept override {
            didRun.store(true);
            while (!token.stop_requested())
                std::this_thread::sleep_for(1ms);

            stopTokenWasHonored.store(true);
        }
    };

    MyTestThread thread;

    REQUIRE_FALSE(thread.isRunning());
    REQUIRE_FALSE(thread.didRun.load());
    REQUIRE(thread.start() == JobThread::StartResult::Started);

    std::this_thread::sleep_for(10ms);
    REQUIRE(thread.isRunning());
    REQUIRE(thread.didRun.load() == true);

    thread.requestStop();
    REQUIRE(thread.join() == true);

    REQUIRE_FALSE(thread.isRunning());
    REQUIRE(thread.stopTokenWasHonored.load() == true);
}

TEST_CASE("JobThread real-time options failure (as non-root)", "[threading][options]")
{
    // This test assumes it's NOT run as root.
    auto opts = JobThreadOptions::realtimeDefault();
    opts.pinToCore = true;
    opts.coreId = 0;

    JobThread thread(opts);
    auto result = thread.start();

    // We expect a failure because we don't have permissions
    bool correctFailure = (result == JobThread::StartResult::SchedulingFailed ||
                           result == JobThread::StartResult::AffinityFailed);


    if (result == JobThread::StartResult::Started) {
        thread.requestStop();
        REQUIRE(thread.join() == true);
    } else {
        REQUIRE(correctFailure);
        REQUIRE_FALSE(thread.isRunning());
        REQUIRE(thread.join() == false);
    }
}

TEST_CASE("JobThread data race stress test (proves mutex)", "[threading][bench][race]")
{
    job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
    constexpr int kNumHammerThreads = 4;
    std::atomic<bool> stopHammer{false};
    std::vector<std::thread> hammer_threads;

    JobThread thread(JobThreadOptions::normal());
    REQUIRE(thread.start() == JobThread::StartResult::Started);
    REQUIRE(thread.isRunning());

    for (int i = 0; i < kNumHammerThreads; ++i) {
        hammer_threads.emplace_back([&, i]() {
            while (!stopHammer.load()) {
                auto opts = JobThreadOptions::normal();
                opts.heartbeat = (i + 1) * 10;
                thread.setOptions(opts);

                if (i % 2 == 0) {
                    thread.setRunFunction(nullptr); // Use default run
                } else {
                    thread.setRunFunction([&]([[maybe_unused]]std::stop_token t){
                        // Do nothing, just override
                    });
                }
            }
        });
    }

    std::this_thread::sleep_for(250ms);

    stopHammer.store(true);
    for(auto& t : hammer_threads) {
        t.join();
    }

    thread.requestStop();
    REQUIRE(thread.join() == true);

    REQUIRE_FALSE(thread.isRunning());
    SUCCEED("Test completed without crashing (data race fix is working)");
}

TEST_CASE("JobThread startup/join latency benchmark", "[threading][bench][latency]")
{
    job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
    constexpr int kNumIterations = 100;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kNumIterations; ++i) {
        JobThread t;
        t.setRunFunction([&](std::stop_token token) {
            while (!token.stop_requested()) {
                std::this_thread::sleep_for(1us);
            }
        });
        REQUIRE(t.start() == JobThread::StartResult::Started);
        t.requestStop();
        REQUIRE(t.join() == true);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double avg_us = static_cast<double>(duration.count()) / kNumIterations;

    JOB_LOG_INFO("[JobThread] Total time for 100 iterations: {} us", duration.count());
    JOB_LOG_INFO("[JobThread] Average start/join latency: {} us per thread" , avg_us);

    REQUIRE(avg_us < 10000.0);
}

