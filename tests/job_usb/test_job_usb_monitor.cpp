#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

#include <job_io_async_thread.h>
#include <job_usb_monitor.h>

#include "../test_spin_till.h"

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Construction / factories
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbMonitor factories create stopped monitors",
          "[job_usb][monitor][factory]")
{
    const auto shared = JobUsbMonitor::createShared();
    const auto unique = JobUsbMonitor::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE_FALSE(shared->isRunning());
    REQUIRE_FALSE(unique->isRunning());
}

//////////////////////////////////////////////////////////
// Block 2: Start validation
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbMonitor cannot start without an async loop",
          "[job_usb][monitor][start][invalid]")
{
    auto monitor = JobUsbMonitor::createShared();

    REQUIRE(monitor);
    REQUIRE_FALSE(monitor->isRunning());

    REQUIRE_FALSE(monitor->start([](JobUsbMonitorAction, std::string_view) {}));

    REQUIRE_FALSE(monitor->isRunning());
}

TEST_CASE("JobUsbMonitor rejects a null async loop",
          "[job_usb][monitor][start][invalid]")
{
    auto monitor = JobUsbMonitor::createShared();

    REQUIRE(monitor);

    REQUIRE_FALSE(monitor->start(threads::JobIoAsyncThread::Ptr{},
                                 [](JobUsbMonitorAction, std::string_view) {}));

    REQUIRE_FALSE(monitor->isRunning());
}

TEST_CASE("JobUsbMonitor rejects a stopped async loop",
          "[job_usb][monitor][start][invalid]")
{
    auto loop =    threads::JobIoAsyncThread::createShared();
    auto monitor = JobUsbMonitor::createShared(loop);

    REQUIRE(loop);
    REQUIRE(monitor);
    REQUIRE_FALSE(loop->isRunning());

    REQUIRE_FALSE(monitor->start([](JobUsbMonitorAction, std::string_view) {}));

    REQUIRE_FALSE(monitor->isRunning());
}

//////////////////////////////////////////////////////////
// Block 3: Lifecycle
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbMonitor starts stops and restarts",
          "[job_usb][monitor][lifecycle]")
{
    auto loop = threads::JobIoAsyncThread::createShared();

    REQUIRE(loop);

    loop->start();

    REQUIRE(loop->isRunning());

    auto monitor = JobUsbMonitor::createShared(loop);

    REQUIRE(monitor);
    REQUIRE_FALSE(monitor->isRunning());

    REQUIRE(monitor->start([](JobUsbMonitorAction, std::string_view) {}));
    REQUIRE(monitor->isRunning());

    // A running monitor must not register itself twice.
    REQUIRE_FALSE(monitor->start([](JobUsbMonitorAction, std::string_view) {}));
    REQUIRE(monitor->isRunning());

    monitor->stop();

    REQUIRE_FALSE(monitor->isRunning());

    // stop() is deliberately idempotent.
    monitor->stop();

    REQUIRE_FALSE(monitor->isRunning());

    // stop() must return the monitor to a restartable state.
    REQUIRE(monitor->start([](JobUsbMonitorAction, std::string_view) {}));
    REQUIRE(monitor->isRunning());

    monitor->stop();

    REQUIRE_FALSE(monitor->isRunning());

    loop->stop();
}

TEST_CASE("JobUsbMonitor may bind its async loop during start",
          "[job_usb][monitor][lifecycle]")
{
    auto loop = threads::JobIoAsyncThread::createShared();
    auto monitor = JobUsbMonitor::createShared();

    REQUIRE(loop);
    REQUIRE(monitor);

    loop->start();

    REQUIRE(loop->isRunning());

    REQUIRE(monitor->start(loop,
                           [](JobUsbMonitorAction, std::string_view) {}));

    REQUIRE(monitor->isRunning());

    monitor->stop();

    REQUIRE_FALSE(monitor->isRunning());

    loop->stop();
}

#ifndef JOB_CI_BUILD

//////////////////////////////////////////////////////////
// Block 4: Real hardware integration
//////////////////////////////////////////////////////////

TEST_CASE("JobUsbMonitor receives a real USB hotplug event",
          "[job_usb][monitor][local][integration]")
{
    auto loop = threads::JobIoAsyncThread::createShared();

    REQUIRE(loop);

    loop->start();

    REQUIRE(loop->isRunning());

    auto monitor = JobUsbMonitor::createShared(loop);

    REQUIRE(monitor);

    std::atomic_bool received{false};

    JobUsbMonitorAction receivedAction = JobUsbMonitorAction::Unknown;
    std::string receivedPath;

    REQUIRE(monitor->start([&](JobUsbMonitorAction action, std::string_view sysfsPath) {
        receivedAction = action;

        // Copy while the udev device backing the view is still alive.
        receivedPath.assign(sysfsPath);

        received.store(true, std::memory_order_release);
    }));

    REQUIRE(monitor->isRunning());

    std::cout << '\n'
              << "============================================================\n"
              << " JobUsbMonitor hardware test\n"
              << " Plug in or unplug a USB device within 60 seconds...\n"
              << "============================================================\n"
              << std::flush;

    REQUIRE(spin_until([&] { return received.load(std::memory_order_acquire); }, std::chrono::seconds(60)));

    REQUIRE(receivedAction != JobUsbMonitorAction::Unknown);
    REQUIRE_FALSE(receivedPath.empty());

    std::cout << "Received USB event: " << receivedPath << '\n';

    monitor->stop();

    REQUIRE_FALSE(monitor->isRunning());

    loop->stop();
}

#endif

} // namespace job::usb::tests