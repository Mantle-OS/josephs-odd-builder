#include <catch2/catch_test_macros.hpp>
#include <thread>

#include <job_timer.h>

using namespace job::core;
using namespace std::chrono_literals;

TEST_CASE("JobTimer basic one-shot behavior", "[timer]") {
    bool fired = false;
    JobTimer timer(1, JobTimer::Clock::now(), 50ms, false, true, [&]() {
        fired = true;
    });

    // Wait slightly longer than interval
    std::this_thread::sleep_for(60ms);

    if (timer.expired(JobTimer::Clock::now()))
        timer.fire();

    REQUIRE(fired == true);
    REQUIRE(timer.isActive() == false);
}

TEST_CASE("JobTimer repeating behavior", "[timer]") {
    int counter = 0;
    JobTimer timer(2, JobTimer::Clock::now(), 20ms, true, true, [&]() {
        counter++;
    });

    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(25ms);
        if (timer.expired(JobTimer::Clock::now()))
            timer.fire();
    }

    REQUIRE(counter >= 3);
    REQUIRE(timer.isActive() == true);
}

TEST_CASE("JobTimer cancel prevents firing", "[timer]") {
    bool fired = false;
    JobTimer timer(3, JobTimer::Clock::now(), 10ms, false, true, [&]() {
        fired = true;
    });

    timer.cancel();
    std::this_thread::sleep_for(20ms);

    if (timer.expired(JobTimer::Clock::now()))
        timer.fire();

    REQUIRE(fired == false);
    REQUIRE(timer.isActive() == false);
}
// CHECKPOINT: v1
