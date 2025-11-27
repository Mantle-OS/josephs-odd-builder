#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

#include <functional>

#include <unistd.h>
#include <sys/epoll.h>

#include <job_io_async_thread.h>
#include <job_logger.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("JobIoAsyncThread processes tasks, timers, and I/O events", "[threading][io_async_loop]")
{
    job::core::JobLogger::instance().setLevel(job::core::LogLevel::Info);
    INFO("Starting JobIoAsyncThread integration test...");

    auto ioLoop = std::make_shared<JobIoAsyncThread>();

    int pipe_fds[2];
    REQUIRE(pipe(pipe_fds) == 0);
    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    std::atomic<bool> io_event_fired{false};
    std::atomic<bool> task_from_io_fired{false};
    std::atomic<bool> timer_fired{false};

    // Read and Edge-Triggered
    ioLoop->registerFD(read_fd, EPOLLIN | EPOLLET,
                       [&](uint32_t events) {
                           INFO("I/O Event Fired!");
                           REQUIRE((events & EPOLLIN));

                           char buf[1];
                           read(read_fd, buf, 1);
                           REQUIRE(buf[0] == 'A');

                           io_event_fired.store(true);
                           ioLoop->post([&] {
                               INFO("Task from I/O Fired!");
                               task_from_io_fired.store(true);
                           });
                       });

    ioLoop->postDelayed([&] {
        INFO("Timer Fired!");
        timer_fired.store(true);
    }, 10ms);

    ioLoop->start();
    REQUIRE(ioLoop->isRunning());

    INFO("Writing to pipe to trigger I/O...");
    ssize_t written = write(write_fd, "A", 1);
    REQUIRE(written == 1);

    int retries = 0;
    while ((!io_event_fired || !task_from_io_fired || !timer_fired) && retries < 200) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    INFO("IO Event Fired: " << io_event_fired.load());
    INFO("Task from IO Fired: " << task_from_io_fired.load());
    INFO("Timer Fired: " << timer_fired.load());

    REQUIRE(io_event_fired.load() == true);
    REQUIRE(task_from_io_fired.load() == true);
    REQUIRE(timer_fired.load() == true);

    ioLoop->stop();
    REQUIRE_FALSE(ioLoop->isRunning());
    close(read_fd);
    close(write_fd);
}

// IMPORTANT
// this is great and all but there could be more to this. However there is already such a LARGE set of tests for the IO/NET stuff that use all this
// CHECKPOINT: v1.0
