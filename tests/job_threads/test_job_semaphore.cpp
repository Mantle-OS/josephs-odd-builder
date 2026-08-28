#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <unistd.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include <job_semaphore.h>

[[nodiscard]] static std::string uniqueSemaphoreName()
{
    static std::atomic<std::uint64_t> sequence{0};
    auto const timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto const threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());

    auto const id = sequence.fetch_add(1, std::memory_order_relaxed);

    return "/jobsem_test_"
           + std::to_string(timestamp)
           + "_"
           + std::to_string(threadId)
           + "_"
           + std::to_string(id);
}

using namespace std::chrono_literals;
using job::threads::JobSem;
using job::threads::JobSemRet;
using job::threads::JobSemFlags;
using job::threads::semiRetToString;
using job::threads::hasFlag;

TEST_CASE("JobSem unnamed basic usage: init, post, wait, value", "[threads][semaphore][usage]")
{
    JobSem sem;

    SECTION("init unnamed semaphore and basic wait/post")
    {
        REQUIRE_FALSE(sem.ready());

        auto r = sem.init(/*value=*/1);
        REQUIRE(r == JobSemRet::OK);
        REQUIRE(sem.ready());

        int v = -1;
        REQUIRE(sem.value(v) == JobSemRet::OK);
        REQUIRE(v == 1);

        // Wait should succeed immediately (value goes 1 -> 0)
        REQUIRE(sem.wait() == JobSemRet::OK);
        REQUIRE(sem.value(v) == JobSemRet::OK);
        REQUIRE(v == 0);

        // Post increments again (0 -> 1)
        REQUIRE(sem.post() == JobSemRet::OK);
        REQUIRE(sem.value(v) == JobSemRet::OK);
        REQUIRE(v == 1);

        // Clean up
        REQUIRE(sem.destroy() == JobSemRet::OK);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("wait N times with shared timeout")
    {
        REQUIRE(sem.init(/*value=*/2) == JobSemRet::OK);

        int v = -1;
        REQUIRE(sem.value(v) == JobSemRet::OK);
        REQUIRE(v == 2);

        // Ask to wait twice, sharing the same total timeout.
        // Because initial value is 2, this should succeed without blocking.
        auto ret = sem.wait(/*value=*/2, /*timeout=*/20ms);
        REQUIRE(ret == JobSemRet::OK);

        REQUIRE(sem.value(v) == JobSemRet::OK);
        REQUIRE(v == 0);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }
}

TEST_CASE("JobSem named semaphores: open, share, and destroy", "[threads][semaphore][usage][named]")
{
    // POSIX sem name must start with '/', so we enforce that here.
    const std::string name = "/jobsem_test_"
                             + std::to_string(::getpid()) + "_"
                             + std::to_string(static_cast<long>(::time(nullptr)));

    JobSem semA;
    JobSem semB;

    semA.setName(name);
    semB.setName(name);

    SECTION("create named semaphore, share between handles, destroy with unlink")
    {
        // Create it, exclusive, auto-unlink on destroy
        auto r_create = semA.open(JobSemFlags::Create
                                      | JobSemFlags::Exclusive
                                      | JobSemFlags::UnlinkOnDestroy,
                                  /*mode=*/0600,
                                  /*value=*/1);
        REQUIRE(r_create == JobSemRet::OK);
        REQUIRE(semA.ready());

        // Open existing named semaphore via second handle
        auto r_open = semB.open(JobSemFlags::None);
        REQUIRE(r_open == JobSemRet::OK);
        REQUIRE(semB.ready());

        int vA = -1, vB = -1;
        REQUIRE(semA.value(vA) == JobSemRet::OK);
        REQUIRE(semB.value(vB) == JobSemRet::OK);
        REQUIRE(vA == 1);
        REQUIRE(vB == 1);

        // Wait on A: value drops to 0, B sees that too
        REQUIRE(semA.wait() == JobSemRet::OK);
        REQUIRE(semA.value(vA) == JobSemRet::OK);
        REQUIRE(semB.value(vB) == JobSemRet::OK);
        REQUIRE(vA == 0);
        REQUIRE(vB == 0);

        // Destroy from A (close + unlink), then close B
        REQUIRE(semA.destroy() == JobSemRet::OK);
        REQUIRE(semB.close()   == JobSemRet::OK);
    }
}

TEST_CASE("JobSem releases a blocked worker after post", "[threads][semaphore][usage][synchronization]")
{
    JobSem sem;
    REQUIRE(sem.init(0, false) == JobSemRet::OK);

    std::atomic<bool> workerStarted{false};
    std::atomic<bool> workerReleased{false};

    std::thread worker([&] {
        workerStarted.store(true, std::memory_order_release);

        auto const result = sem.wait();

        if (result == JobSemRet::OK)
            workerReleased.store(true, std::memory_order_release);
    });

    while (!workerStarted.load(std::memory_order_acquire))
        std::this_thread::yield();

    REQUIRE_FALSE(workerReleased.load(std::memory_order_acquire));

    REQUIRE(sem.post() == JobSemRet::OK);

    worker.join();

    REQUIRE(workerReleased.load(std::memory_order_acquire));
    REQUIRE(sem.destroy() == JobSemRet::OK);
}

TEST_CASE("JobSem timed wait succeeds when another thread posts", "[threads][semaphore][usage][timeout]")
{
    JobSem sem;
    REQUIRE(sem.init(0, false) == JobSemRet::OK);

    std::thread producer([&] {
        std::this_thread::sleep_for(5ms);
        REQUIRE(sem.post() == JobSemRet::OK);
    });

    REQUIRE(sem.wait(100ms) == JobSemRet::OK);

    producer.join();

    REQUIRE(sem.destroy() == JobSemRet::OK);
}

TEST_CASE("JobSem can be initialized again after destruction", "[threads][semaphore][lifecycle][edge]")
{
    JobSem sem;

    REQUIRE(sem.init(1, false) == JobSemRet::OK);
    REQUIRE(sem.destroy() == JobSemRet::OK);
    REQUIRE_FALSE(sem.ready());

    REQUIRE(sem.init(2, false) == JobSemRet::OK);
    REQUIRE(sem.ready());

    int value = -1;
    REQUIRE(sem.value(value) == JobSemRet::OK);
    REQUIRE(value == 2);

    REQUIRE(sem.destroy() == JobSemRet::OK);
}

TEST_CASE("JobSem exclusive creation rejects an existing named semaphore","[threads][semaphore][named][edge][exclusive]")
{
    auto const name = uniqueSemaphoreName();

    JobSem owner;
    JobSem duplicate;

    owner.setName(name);
    duplicate.setName(name);

    REQUIRE(
        owner.open(JobSemFlags::Create | JobSemFlags::Exclusive | JobSemFlags::UnlinkOnDestroy,
                   0600, 1) == JobSemRet::OK
        );

    REQUIRE(
        duplicate.open( JobSemFlags::Create | JobSemFlags::Exclusive,
            0600, 1) == JobSemRet::Exists
        );

    REQUIRE(owner.destroy() == JobSemRet::OK);
}

TEST_CASE("JobSem unlink-on-destroy removes the named semaphore", "[threads][semaphore][named][lifecycle][edge]")
{
    auto const name = uniqueSemaphoreName();

    {
        JobSem owner;
        owner.setName(name);

        REQUIRE(
            owner.open(JobSemFlags::Create | JobSemFlags::Exclusive | JobSemFlags::UnlinkOnDestroy,
                       0600, 1) == JobSemRet::OK
            );
    }

    JobSem probe;
    probe.setName(name);

    REQUIRE(probe.open(JobSemFlags::None) == JobSemRet::NotFound);
}

TEST_CASE("JobSem named handles share semaphore state", "[threads][semaphore][named][usage][synchronization]")
{
    auto const name = uniqueSemaphoreName();

    JobSem producer;
    JobSem consumer;

    producer.setName(name);
    consumer.setName(name);

    REQUIRE(
        producer.open(JobSemFlags::Create | JobSemFlags::Exclusive | JobSemFlags::UnlinkOnDestroy,
            0600, 0 ) == JobSemRet::OK
        );

    REQUIRE(consumer.open(JobSemFlags::None) == JobSemRet::OK);

    std::atomic<JobSemRet> waitResult{JobSemRet::Unknown};

    std::thread worker([&] {
        waitResult.store(consumer.wait(100ms), std::memory_order_release);
    });

    std::this_thread::sleep_for(5ms);

    REQUIRE(producer.post() == JobSemRet::OK);

    worker.join();

    REQUIRE(waitResult.load(std::memory_order_acquire) == JobSemRet::OK);

    REQUIRE(consumer.close() == JobSemRet::OK);
    REQUIRE(producer.destroy() == JobSemRet::OK);
}


TEST_CASE("JobSem edge cases: invalid operations and timeouts", "[threads][semaphore][edge]")
{
    JobSem sem;

    SECTION("operations on an uninitialized semaphore return NotReady")
    {
        int value = -1;

        REQUIRE_FALSE(sem.ready());
        REQUIRE(sem.wait() == JobSemRet::NotReady);
        REQUIRE(sem.wait(1ms) == JobSemRet::NotReady);
        REQUIRE(sem.post() == JobSemRet::NotReady);
        REQUIRE(sem.value(value) == JobSemRet::NotReady);
        REQUIRE(sem.destroy() == JobSemRet::NotReady);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("double init is Invalid")
    {
        REQUIRE(sem.init(0, false) == JobSemRet::OK);
        REQUIRE(sem.ready());

        REQUIRE(sem.init(0, false) == JobSemRet::Invalid);
        REQUIRE(sem.ready());

        REQUIRE(sem.destroy() == JobSemRet::OK);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("timed wait times out when value is zero")
    {
        REQUIRE(sem.init(0, false) == JobSemRet::OK);

        auto const start = std::chrono::steady_clock::now();
        auto const result = sem.wait(5ms);
        auto const end = std::chrono::steady_clock::now();

        REQUIRE(result == JobSemRet::Timeout);

        auto const elapsed = end - start;
        REQUIRE(elapsed >= 1ms);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("steady-clock duration wrapper times out")
    {
        REQUIRE(sem.init(0, false) == JobSemRet::OK);

        REQUIRE(sem.wait(JobSem::ClockDuration(5ms)) == JobSemRet::Timeout);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("negative timeout is Invalid")
    {
        REQUIRE(sem.init(0, false) == JobSemRet::OK);

        REQUIRE(sem.wait(JobSem::PointDuration{-1}) == JobSemRet::Invalid);

        int value = -1;
        REQUIRE(sem.value(value) == JobSemRet::OK);
        REQUIRE(value == 0);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("multi-wait rejects zero and negative counts")
    {
        REQUIRE(sem.init(2, false) == JobSemRet::OK);

        REQUIRE(sem.wait(0, 10ms) == JobSemRet::Invalid);
        REQUIRE(sem.wait(-1, 10ms) == JobSemRet::Invalid);

        // Invalid requests must not consume any semaphore units.
        int value = -1;
        REQUIRE(sem.value(value) == JobSemRet::OK);
        REQUIRE(value == 2);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("close rejects an unnamed semaphore")
    {
        REQUIRE(sem.init(1, false) == JobSemRet::OK);
        REQUIRE(sem.ready());

        REQUIRE(sem.close() == JobSemRet::Invalid);
        REQUIRE(sem.ready());

        int value = -1;
        REQUIRE(sem.value(value) == JobSemRet::OK);
        REQUIRE(value == 1);

        REQUIRE(sem.destroy() == JobSemRet::OK);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("unlink rejects a semaphore without a named handle")
    {
        REQUIRE(sem.unlink() == JobSemRet::Invalid);
        sem.setName(uniqueSemaphoreName());
        // Merely assigning a name does not make this an opened named
        // semaphore under the current JobSem ownership contract.
        REQUIRE(sem.unlink() == JobSemRet::Invalid);
    }

    SECTION("named semaphore must have a POSIX-style name")
    {
        sem.setName("no_leading_slash");
        REQUIRE(sem.open(JobSemFlags::Create, 0600, 1) == JobSemRet::Invalid);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("open rejects an already initialized semaphore")
    {
        REQUIRE(sem.init(1, false) == JobSemRet::OK);

        sem.setName(uniqueSemaphoreName());

        REQUIRE(sem.open(JobSemFlags::Create, 0600, 1) == JobSemRet::Invalid);

        REQUIRE(sem.ready());
        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("second open on the same wrapper is Invalid")
    {
        auto const name = uniqueSemaphoreName();
        sem.setName(name);

        REQUIRE(
            sem.open( JobSemFlags::Create | JobSemFlags::Exclusive | JobSemFlags::UnlinkOnDestroy,
                0600, 1) == JobSemRet::OK
            );

        REQUIRE(sem.ready());
        REQUIRE(sem.open(JobSemFlags::None) == JobSemRet::Invalid);
        REQUIRE(sem.ready());

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("semiRetToString covers every public result")
    {
        REQUIRE(std::string(semiRetToString(JobSemRet::OK))             == "OK" );
        REQUIRE(std::string(semiRetToString(JobSemRet::NotReady))       == "NotReady" );
        REQUIRE(std::string(semiRetToString(JobSemRet::Invalid))        == "Invalid" );
        REQUIRE(std::string(semiRetToString(JobSemRet::Timeout))        == "Timeout" );
        REQUIRE(std::string(semiRetToString(JobSemRet::Interrupted))    == "Interrupted" );
        REQUIRE(std::string(semiRetToString(JobSemRet::WouldBlock))     == "WouldBlock" );
        REQUIRE(std::string(semiRetToString(JobSemRet::Exists))         == "Exists" );
        REQUIRE(std::string(semiRetToString(JobSemRet::NotFound))       == "NotFound");
        REQUIRE(std::string(semiRetToString(JobSemRet::NoMemory))       == "NoMemory");
        REQUIRE(std::string(semiRetToString(JobSemRet::Permission))     == "Permission");
        REQUIRE(std::string(semiRetToString(JobSemRet::Unknown))        == "Unknown");
    }

    SECTION("JobSemFlags bit operations and hasFlag helper")
    {
        JobSemFlags flags = JobSemFlags::None;

        REQUIRE_FALSE(hasFlag(flags, JobSemFlags::Create));
        REQUIRE_FALSE(hasFlag(flags, JobSemFlags::Exclusive));
        REQUIRE_FALSE( hasFlag(flags, JobSemFlags::UnlinkOnDestroy));

        flags |= JobSemFlags::Create;

        REQUIRE(hasFlag(flags, JobSemFlags::Create));
        REQUIRE_FALSE(hasFlag(flags, JobSemFlags::Exclusive));

        flags |= JobSemFlags::Exclusive;

        REQUIRE(hasFlag(flags, JobSemFlags::Create));
        REQUIRE(hasFlag(flags, JobSemFlags::Exclusive));
        REQUIRE_FALSE(hasFlag(flags, JobSemFlags::UnlinkOnDestroy));

        flags |= JobSemFlags::UnlinkOnDestroy;

        REQUIRE(hasFlag(flags, JobSemFlags::Create));
        REQUIRE(hasFlag(flags, JobSemFlags::Exclusive));
        REQUIRE(hasFlag(flags, JobSemFlags::UnlinkOnDestroy));

        auto const createAndExclusive = JobSemFlags::Create | JobSemFlags::Exclusive;
        REQUIRE( hasFlag(createAndExclusive, JobSemFlags::Create));
        REQUIRE(hasFlag(createAndExclusive, JobSemFlags::Exclusive));
        REQUIRE_FALSE(hasFlag(createAndExclusive, JobSemFlags::UnlinkOnDestroy));
    }
}



#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobSem micro-benchmark: post+wait pair on unnamed semaphore",
          "[threads][semaphore][benchmark]")
{
    JobSem sem;
    REQUIRE(sem.init(/*value=*/1) == JobSemRet::OK);
    REQUIRE(sem.ready());

    // Because initial value is 1 and we always do post() before wait(), the wait() never actually blocks here.
    BENCHMARK("JobSem post() + wait() pair (non-blocking)") {
        (void)sem.post();
        (void)sem.wait();
    };

    REQUIRE(sem.destroy() == JobSemRet::OK);
}
#endif
