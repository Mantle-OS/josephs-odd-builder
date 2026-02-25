#include <catch2/catch_test_macros.hpp>
#include <job_timer.h>

#include "../test_spin_till.h"

using namespace job::core;
using namespace std::chrono_literals;

TEST_CASE("JobTimer basic one-shot behavior", "[timer]")
{
    bool fired = false;
    JobTimer timer(1, JobTimer::Clock::now(), 50ms, false, true, [&]() {
        fired = true;
    });
    REQUIRE(spin_until([&]{
        if (timer.expired(JobTimer::Clock::now())) timer.fire();
        return fired;
    }, 200ms));
    REQUIRE_FALSE(timer.isActive());
}

TEST_CASE("JobTimer repeating behavior", "[timer]")
{
    int counter = 0;
    JobTimer timer(2, JobTimer::Clock::now(), 20ms, true, true, [&]() {
        counter++;
    });

    REQUIRE(spin_until([&]{
        if (timer.expired(JobTimer::Clock::now()))
            timer.fire();

        return counter >= 3;
    }, 250ms));

    REQUIRE(timer.isActive());

}

TEST_CASE("JobTimer cancel prevents firing", "[timer]") {
    bool fired = false;
    JobTimer timer(3, JobTimer::Clock::now(), 10ms, false, true, [&]() {
        fired = true;
    });

    timer.cancel();
    bool seen = spin_until([&]{
        if (timer.expired(JobTimer::Clock::now()))
            timer.fire();
        return fired;
    }, 50ms);

    REQUIRE_FALSE(seen);
    REQUIRE_FALSE(timer.isActive());
}

