#include "job_latch.h"
#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

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
        MyTestThread() :
            JobThread(JobThreadOptions::normal())
        {

        }
    protected:
        void run(std::stop_token token) noexcept override
        {
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

// Edge Cases
TEST_CASE("JobThread edge cases: RAII cleanup without explicit join", "[threading][edge]")
{
    SECTION("Destructor requests stop and joins active thread automatically") {
        std::atomic<bool> didRun{false};
        std::atomic<bool> stopHonored{false};

        {
            JobThread thread;
            thread.setRunFunction([&](std::stop_token token) {
                didRun.store(true);
                while (!token.stop_requested()) {
                    std::this_thread::sleep_for(1ms);
                }
                stopHonored.store(true);
            });

            REQUIRE(thread.start() == JobThread::StartResult::Started);
            std::this_thread::sleep_for(5ms);
        } // Block exit triggers ~JobThread() -> requestStop() + join()

        REQUIRE(didRun.load());
        REQUIRE(stopHonored.load());
    }

    SECTION("Destructor handles unstarted thread gracefully") {
        REQUIRE_NOTHROW([]() {
            JobThread thread;
            thread.setOptions(JobThreadOptions::normal());
            // Exit scope without calling start()
        }());
    }
}

TEST_CASE("JobThread edge cases: State recovery after start failure", "[threading][edge]")
{
    SECTION("Flag m_starting is cleared after failed start, allowing re-start") {
        JobThreadOptions invalidOpts = JobThreadOptions::normal();
        invalidOpts.realtime = true;
        invalidOpts.policy = SchedulingPolicy::Other; // Invalid combination

        JobThread thread(invalidOpts);
        thread.setRunFunction([](std::stop_token token) {
            while (!token.stop_requested()) {
                std::this_thread::sleep_for(100us);
            }
        });

        // First attempt fails during options validation/scheduling
        auto res1 = thread.start();
        REQUIRE(res1 == JobThread::StartResult::SchedulingFailed);
        REQUIRE_FALSE(thread.isRunning());

        // Fix options
        thread.setOptions(JobThreadOptions::normal());

        // Second attempt must succeed (proves m_starting was cleared and not left stuck)
        auto res2 = thread.start();
        REQUIRE(res2 == JobThread::StartResult::Started);
        REQUIRE(thread.isRunning());

        thread.requestStop();
        REQUIRE(thread.join() == true);
    }
}

TEST_CASE("JobThread edge cases: Non-realtime thread core pinning", "[threading][edge]")
{
    JobThreadOptions opts = JobThreadOptions::normal();
    opts.realtime = false;
    opts.pinToCore = true;
    opts.coreId = 0; // Pin to CPU core 0

    JobThread thread(opts);
    std::atomic<bool> ran{false};
    thread.setRunFunction([&](std::stop_token token) {
        ran.store(true, std::memory_order_release);
        while (!token.stop_requested()) {
            std::this_thread::sleep_for(100us);
        }
    });

    // Validates that applyAffinity runs even when realtime=false
    auto result = thread.start();
    if (result == JobThread::StartResult::Started) {
        REQUIRE(thread.isRunning());
        thread.requestStop();
        REQUIRE(thread.join() == true);
        REQUIRE(ran.load(std::memory_order_acquire));
    } else {
        // If system restricts affinity calls, it should fail with AffinityFailed, not silently ignore
        REQUIRE(result == JobThread::StartResult::AffinityFailed);
        REQUIRE_FALSE(thread.isRunning());
        REQUIRE_FALSE(thread.join());
    }
}

TEST_CASE("JobThread edge cases: Out-of-bounds parameter guards", "[threading][edge]")
{
    SECTION("Unbound coreId validation") {
        JobThreadOptions opts = JobThreadOptions::normal();
        opts.pinToCore = true;
        opts.coreId = JobThreadOptions::kCoreUnbound; // 0xFF

        JobThread thread(opts);
        // Invalid core ID must trigger AffinityFailed during startup
        REQUIRE(thread.start() == JobThread::StartResult::AffinityFailed);
        REQUIRE_FALSE(thread.isRunning());
        REQUIRE_FALSE(thread.join());
    }

    SECTION("Invalid priority range validation") {
        JobThreadOptions opts = JobThreadOptions::normal();
        opts.priority = 105; // Maximum valid priority is 99

        JobThread thread(opts);
        REQUIRE(thread.start() == JobThread::StartResult::SchedulingFailed);
        REQUIRE_FALSE(thread.isRunning());
        REQUIRE_FALSE(thread.join());
    }
}

TEST_CASE("JobThread edge cases: Rapid start/stop churn", "[threading][edge][stress]")
{
    constexpr int kIterations = 50;
    JobThread thread;

    for (int i = 0; i < kIterations; ++i) {
        std::atomic<bool> ran{false};

        thread.setRunFunction([&ran](std::stop_token token) {
            ran.store(true, std::memory_order_release);
            while (!token.stop_requested()) {
                std::this_thread::sleep_for(10us);
            }
        });

        REQUIRE(thread.start() == JobThread::StartResult::Started);
        REQUIRE(thread.start() == JobThread::StartResult::AlreadyRunning);

        thread.requestStop();

        // join() guarantees the threadEntry function has completely returned
        REQUIRE(thread.join() == true);
        REQUIRE_FALSE(thread.join()); // Second join must safely return false

        REQUIRE(ran.load(std::memory_order_acquire) == true);
    }
}

TEST_CASE("JobThread permits only one active join operation", "[threading][job-thread][join][edge]")
{
    JobThread thread;

    JobLatch workerEntered{1};
    JobLatch releaseWorker{1};

    thread.setRunFunction([&](std::stop_token token) {
        workerEntered.countDown();
        releaseWorker.wait();
        while (!token.stop_requested())
            std::this_thread::yield();
    });

    REQUIRE(thread.start() == JobThread::StartResult::Started);

    workerEntered.wait();

    std::atomic<bool> firstJoinResult{false};
    JobLatch joinCallerReady{1};

    std::thread joiningThread([&] {
        joinCallerReady.countDown();
        firstJoinResult.store(thread.join(), std::memory_order_release);
    });

    joinCallerReady.wait();

    REQUIRE_FALSE(thread.join());
    REQUIRE(thread.start() == JobThread::StartResult::AlreadyRunning);

    thread.requestStop();
    releaseWorker.countDown();

    joiningThread.join();

    REQUIRE(firstJoinResult.load(std::memory_order_acquire));
    REQUIRE_FALSE(thread.isRunning());
    REQUIRE_FALSE(thread.join());
}


#ifndef JOB_CI_BUILD
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
#endif

#ifdef JOB_TEST_BENCHMARKS
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
                    thread.setRunFunction([&]([[maybe_unused]] std::stop_token t){
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

    BENCHMARK("JobThread start + requestStop + join cycle") {
        JobThread t;
        t.setRunFunction([&](std::stop_token token) {
            while (!token.stop_requested()) {
                std::this_thread::sleep_for(1us);
            }
        });
        (void)t.start();
        t.requestStop();
        return t.join();
    };
}

#endif