#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_semaphore.h>

#include <chrono>
#include <string>
#include <thread>
#include <unistd.h>

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


TEST_CASE("JobSem edge cases: invalid operations and timeouts", "[threads][semaphore][edge]")
{
    JobSem sem;

    SECTION("destroy on not-ready returns NotReady")
    {
        REQUIRE(sem.destroy() == JobSemRet::NotReady);
    }

    SECTION("double init is Invalid")
    {
        REQUIRE(sem.init(0) == JobSemRet::OK);
        REQUIRE(sem.init(0) == JobSemRet::Invalid);
        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("wait on uninitialized semaphore returns NotReady")
    {
        REQUIRE(sem.wait() == JobSemRet::NotReady);
    }

    SECTION("timed wait times out when value is zero")
    {
        REQUIRE(sem.init(/*value=*/0) == JobSemRet::OK);

        auto start = std::chrono::steady_clock::now();
        auto ret = sem.wait(5ms);
        auto end = std::chrono::steady_clock::now();

        REQUIRE(ret == JobSemRet::Timeout);

        // Just sanity-check we actually waited at least *some* time.
        auto elapsed = end - start;
        REQUIRE(elapsed >= 1ms);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("steady-clock wait wrapper also times out")
    {
        REQUIRE(sem.init(/*value=*/0) == JobSemRet::OK);

        auto ret = sem.wait(JobSem::ClockDuration(5ms));
        REQUIRE(ret == JobSemRet::Timeout);

        REQUIRE(sem.destroy() == JobSemRet::OK);
    }

    SECTION("named semaphore must have POSIX-style name")
    {
        sem.setName("no_leading_slash");
        auto r = sem.open(JobSemFlags::Create, 0600, 1);
        REQUIRE(r == JobSemRet::Invalid);
        REQUIRE_FALSE(sem.ready());
    }

    SECTION("semiRetToString covers basic mapping")
    {
        REQUIRE(std::string(semiRetToString(JobSemRet::OK))          == "OK");
        REQUIRE(std::string(semiRetToString(JobSemRet::Timeout))     == "Timeout");
        REQUIRE(std::string(semiRetToString(JobSemRet::NotReady))    == "NotReady");
        REQUIRE(std::string(semiRetToString(JobSemRet::Invalid))     == "Invalid");
        REQUIRE(std::string(semiRetToString(JobSemRet::Interrupted)) == "Interrupted");
        REQUIRE(std::string(semiRetToString(JobSemRet::Unknown))     == "Unknown");
    }

    SECTION("JobSemFlags bit operations and hasFlag helper")
    {
        JobSemFlags f = JobSemFlags::None;
        REQUIRE_FALSE(hasFlag(f, JobSemFlags::Create));
        REQUIRE_FALSE(hasFlag(f, JobSemFlags::Exclusive));
        REQUIRE_FALSE(hasFlag(f, JobSemFlags::UnlinkOnDestroy));

        f |= JobSemFlags::Create;
        REQUIRE(hasFlag(f, JobSemFlags::Create));
        REQUIRE_FALSE(hasFlag(f, JobSemFlags::Exclusive));

        f |= JobSemFlags::Exclusive;
        REQUIRE(hasFlag(f, JobSemFlags::Exclusive));

        auto both = JobSemFlags::Create | JobSemFlags::Exclusive;
        REQUIRE(hasFlag(both, JobSemFlags::Create));
        REQUIRE(hasFlag(both, JobSemFlags::Exclusive));
        REQUIRE_FALSE(hasFlag(both, JobSemFlags::UnlinkOnDestroy));
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
