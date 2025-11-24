// test_sporadic_sched.cpp
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#include <job_logger.h>
#include <job_sporadic_scheduler.h>
#include <descr/job_sporadic_descriptor.h>
// Optional: pool smoke test
#include <job_thread_pool.h>
#include <job_thread_options.h>

using namespace job::threads;
using namespace std::chrono_literals;

static JobSporadicDescriptor::Ptr makeSporadic(uint64_t id,
                                               std::chrono::steady_clock::time_point dl,
                                               std::chrono::microseconds wcet,
                                               int prio = 0)
{
    return std::make_shared<JobSporadicDescriptor>(id, prio, dl, wcet);
}

TEST_CASE("JobSporadicScheduler EDF admission: feasible pair is admitted and ordered by deadline", "[sporadic][admit]") {
    // capacity = 1
    JobSporadicScheduler sched;
    const auto base = std::chrono::steady_clock::now();

    // 100 ms slack
    auto j1 = makeSporadic(1, base + 900ms, 800ms);
    // add slack so C1+C2 fits by D2 after elapsed time
    // auto j2 = makeSporadic(2, base + 1700ms, 800ms);
    auto j2 = makeSporadic(2, base + 1600ms, 799ms);

    REQUIRE(sched.admit(1, *j1));
    sched.enqueue(j1);

    REQUIRE(sched.admit(1, *j2));
    sched.enqueue(j2);

    auto n1 = sched.next(0);
    REQUIRE(n1.has_value());
    REQUIRE(*n1 == 1);

    auto n2 = sched.next(0);
    REQUIRE(n2.has_value());
    REQUIRE(*n2 == 2);

    sched.complete(1, true);
    sched.complete(2, true);
}

TEST_CASE("JobSporadicScheduler EDF admission: impossible single job is rejected", "[sporadic][admit]")
{
    JobSporadicScheduler sched;
    const auto now = std::chrono::steady_clock::now();

    // WCET > available time -> reject
    auto bad = makeSporadic(10, now + 50ms, 60ms);
    REQUIRE_FALSE(sched.admit(1, *bad));
}

TEST_CASE("JobSporadicScheduler EDF admission counts in-flight work", "[sporadic][inflight]")
{
    JobSporadicScheduler sched;
    const auto now = std::chrono::steady_clock::now();

    // J1 admitted and dispatched (moves to in-flight)
    auto j1 = makeSporadic(100, now + 1000ms, 800ms);
    REQUIRE(sched.admit(1, *j1));
    sched.enqueue(j1);
    auto id = sched.next(0);
    REQUIRE(id.has_value());
    REQUIRE(*id == 100);

    // New J2 would exceed demand by its deadline -> should be rejected
    // 800 + 300 > 1000 -> Infeasible lol !!
    auto j2 = makeSporadic(101, now + 1000ms, 300ms);
    REQUIRE_FALSE(sched.admit(1, *j2));

    sched.complete(100, true);
}

TEST_CASE("JobSporadicScheduler Reservation closes the admit→enqueue race", "[sporadic][reservation]")
{
    JobSporadicScheduler sched;
    const auto now = std::chrono::steady_clock::now();

    // Two jobs that together don't fit by the earlier deadline.
    auto a = makeSporadic(200, now + 500ms, 400ms);
    // 400 + 200 > 500 => cannot both be admitted
    auto b = makeSporadic(201, now + 500ms, 200ms);

    // Admit first (create reservation)
    REQUIRE(sched.admit(1, *a));

    // Second admit should fail even though nothing is queued yet (reservation counted)
    REQUIRE_FALSE(sched.admit(1, *b));

    // Enqueue the reserved one (consumes reservation)
    sched.enqueue(a);

    auto n = sched.next(0);
    REQUIRE(n.has_value());
    REQUIRE(*n == 200);
    sched.complete(200, true);
}

TEST_CASE("JobSporadicScheduler Ad-hoc enqueue path rechecks admission without prior admit()", "[sporadic][adhoc]")
{
    JobSporadicScheduler sched;
    const auto now = std::chrono::steady_clock::now();

    // Feasible single job enqueued directly
    auto j = makeSporadic(300, now + 400ms, 100ms);
    // No prior admit(). enqueue() will verify internally.
    sched.enqueue(j);

    auto n = sched.next(0);
    REQUIRE(n.has_value());
    REQUIRE(*n == 300);
    sched.complete(300, true);
}

TEST_CASE("JobSporadicScheduler Stop wakes next() when queue is empty", "[sporadic][stop]")
{
    JobSporadicScheduler sched;
    std::atomic<bool> awakened{false};

    std::thread waiter([&]{
         // block until stop()
        auto id = sched.next(0);
        REQUIRE_FALSE(id.has_value());
        awakened.store(true);
    });

    std::this_thread::sleep_for(20ms);
    sched.stop();

    waiter.join();
    REQUIRE(awakened.load());
}

TEST_CASE("ThreadPool + JobSporadicScheduler run in EDF order (1 worker)", "[pool][sporadic]")
{
    auto sched = std::make_shared<JobSporadicScheduler>();
    auto pool  = ThreadPool::create(sched, /*threadCount=*/1, JobThreadOptions::normal());

    REQUIRE(pool != nullptr);

    const auto now = std::chrono::steady_clock::now();
    std::mutex guard;
    std::vector<uint64_t> ran;

    auto push_id = [&](uint64_t id){
        std::lock_guard<std::mutex> lk(guard);
        ran.push_back(id);
    };

    auto d1 = makeSporadic(401, now + 300ms, 50ms);
    auto d2 = makeSporadic(402, now + 100ms, 10ms);
    auto d3 = makeSporadic(403, now + 200ms, 20ms);

    auto f1 = pool->submit(d1, [&, id=401]{ push_id(id); });
    auto f2 = pool->submit(d2, [&, id=402]{ push_id(id); });
    auto f3 = pool->submit(d3, [&, id=403]{ push_id(id); });

    f1.wait(); f2.wait(); f3.wait();
    pool->waitForIdle();

    REQUIRE(ran.size() == 3);
    REQUIRE(ran[0] == 402);
    REQUIRE(ran[1] == 403);
    REQUIRE(ran[2] == 401);
}

TEST_CASE("JobSporadicScheduler admits tasks based on m_capacity > 1", "[threading][scheduler][sporadic][multi_core]")
{
    auto sched = std::make_shared<JobSporadicScheduler>(2);
    auto now = std::chrono::steady_clock::now();

    INFO("Testing with m_capacity = 2");

    auto descA = makeSporadic(1, now + 100ms, std::chrono::microseconds(80000));
    REQUIRE(sched->admit(2, *descA) == true);
    sched->enqueue(descA);
    INFO("Admitted Task A (80ms wcet @ 100ms dl)");

    auto descB = makeSporadic(2, now + 100ms, std::chrono::microseconds(80000));
    REQUIRE(sched->admit(2, *descB) == true);
    sched->enqueue(descB);
    INFO("Admitted Task B (80ms wcet @ 100ms dl)");

    auto descC_fail = makeSporadic(3, now + 100ms, std::chrono::microseconds(80000));
    REQUIRE(sched->admit(2, *descC_fail) == false);
    INFO("Correctly rejected Task C (capacity exceeded)");

    auto descD_pass = makeSporadic(4, now + 50ms, std::chrono::microseconds(10000));
    REQUIRE(sched->admit(2, *descD_pass) == true);
    INFO("Correctly admitted Task D with earlier deadline");
}
