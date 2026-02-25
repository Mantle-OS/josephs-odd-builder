#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

#include <job_thread.h>
#include <job_thread_pool.h>
#include <job_thread_watcher.h>
#include <sched/job_fifo_scheduler.h>


using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("ThreadWatcher create, start, and stop", "[threading][thread_watcher]")
{
    auto watcher = std::make_shared<ThreadWatcher>();
    REQUIRE_FALSE(watcher->isRunning());

    watcher->start();
    REQUIRE(watcher->isRunning());

    watcher->stop();
    REQUIRE_FALSE(watcher->isRunning());
}

TEST_CASE("ThreadWatcher detects and stops a stuck thread", "[threading][thread_watcher]")
{
    auto scheduler = std::make_shared<FifoScheduler>();
    auto watcher = std::make_shared<ThreadWatcher>();
    auto pool = ThreadPool::create(scheduler, 2);

    watcher->attachPool(pool);
    watcher->attachScheduler(scheduler);

    std::atomic<bool> thread_was_stopped = false;

    // hang out for a couple if ms I hate this -- it works but feels dirty
    auto stuck_worker = std::make_shared<JobThread>();
    stuck_worker->setRunFunction([&](std::stop_token token) {
        while (!token.stop_requested())
            std::this_thread::sleep_for(10ms);

        thread_was_stopped.store(true);
    });

    REQUIRE(stuck_worker->start() == JobThread::StartResult::Started);
    REQUIRE(stuck_worker->isRunning());

    watcher->addThread(stuck_worker, 50ms, 1);

    watcher->setSummaryInterval(1s);
    watcher->start();
    REQUIRE(watcher->isRunning());

    int retries = 0;
    while (stuck_worker->isRunning() && retries < 20) {
        std::this_thread::sleep_for(10ms);
        retries++;
    }

    REQUIRE_FALSE(stuck_worker->isRunning());

    REQUIRE(thread_was_stopped.load() == true);

    watcher->stop();
    pool->shutdown();
}

TEST_CASE("ThreadWatcher handles a thread that finishes normally", "[threading][thread_watcher][pass]")
{
    auto watcher = std::make_shared<ThreadWatcher>();
    std::atomic<bool> did_run = false;
    auto quick_worker = std::make_shared<JobThread>();
    quick_worker->setRunFunction([&]([[maybe_unused]]std::stop_token token){
        std::this_thread::sleep_for(20ms); // Do some "work"
        did_run.store(true);
    });

    watcher->addThread(quick_worker, 1000ms, 1);
    watcher->start();
    REQUIRE(quick_worker->start() == JobThread::StartResult::Started);

    std::this_thread::sleep_for(100ms);

    REQUIRE(did_run.load() == true);
    // finish line around the corner
    REQUIRE_FALSE(quick_worker->isRunning());

    watcher->stop();
    SUCCEED("Watcher handled a fast-finishing thread gracefully.");
}


TEST_CASE("ThreadWatcher can add a new thread while running", "[threading][thread_watcher][concurrency]")
{
    auto watcher = std::make_shared<ThreadWatcher>();
    watcher->start(); // Start the watcher FIRST
    REQUIRE(watcher->isRunning());
    std::this_thread::sleep_for(20ms);

    std::atomic<bool> thread_was_stopped = false;
    auto stuck_worker = std::make_shared<JobThread>();
    stuck_worker->setRunFunction([&](std::stop_token token) {
        while (!token.stop_requested())
            std::this_thread::sleep_for(10ms);
        thread_was_stopped.store(true);
    });

    REQUIRE(stuck_worker->start() == JobThread::StartResult::Started);

    watcher->addThread(stuck_worker, 50ms, 1);
    int retries = 0;
    while (stuck_worker->isRunning() && retries < 20) {
        std::this_thread::sleep_for(10ms);
        retries++;
    }
    REQUIRE_FALSE(stuck_worker->isRunning());
    REQUIRE(thread_was_stopped.load() == true);

    watcher->stop();
}

