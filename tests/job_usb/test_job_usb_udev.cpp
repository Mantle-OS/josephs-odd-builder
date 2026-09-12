#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <future>
#include <vector>

#include <job_fifo_ctx.h>
#include <job_usb_udev.h>

#include "test_job_usb_fixtures.h"

namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobUsbUdev creates a valid Linux udev resolver",
          "[usb][udev][usage]")
{
    JobUsbUdev udev;
    REQUIRE(udev.valid());
}

TEST_CASE("JobUsbUdev factories create valid resolvers",
          "[usb][udev][factory]")
{
    auto shared = JobUsbUdev::createShared();

    REQUIRE(shared);
    REQUIRE(shared->valid());

    auto unique = JobUsbUdev::createUniq();

    REQUIRE(unique);
    REQUIRE(unique->valid());
}

#ifndef JOB_CI_BUILD

TEST_CASE("JobUsbUdev resolves a libusb device to its sysfs path",
          "[usb][udev][usage][hardware]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbUdev udev;

    REQUIRE(udev.valid());

    const auto path = udev.sysfsPath(fixture.busNumber(),
                                     fixture.deviceAddress());

    REQUIRE(path);
    REQUIRE_FALSE(path->empty());
    REQUIRE(path->starts_with("/sys/"));
}

TEST_CASE("JobUsbUdev reads properties from a resolved USB device",
          "[usb][udev][usage][hardware][property]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbUdev udev;

    const auto deviceType = udev.property(fixture.busNumber(),
                                          fixture.deviceAddress(),
                                          "DEVTYPE");

    REQUIRE(deviceType);
    REQUIRE(*deviceType == "usb_device");
}

#endif // JOB_CI_BUILD

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbUdev returns no result for an invalid USB location",
          "[usb][udev][edge]")
{
    JobUsbUdev udev;

    REQUIRE(udev.valid());

    REQUIRE_FALSE(udev.sysfsPath(0, 0));
    REQUIRE_FALSE(udev.property(0, 0, "DEVTYPE"));
}

#ifndef JOB_CI_BUILD

TEST_CASE("JobUsbUdev returns no result for an unknown property",
          "[usb][udev][edge][hardware][property]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbUdev udev;
    const auto value = udev.property(
        fixture.busNumber(),
        fixture.deviceAddress(),
        "EULER_IS_THE_SLIGHTLY_DRUNK_COUSIN_OF_RK4"
        );

    REQUIRE_FALSE(value);
}

TEST_CASE("JobUsbUdev safely resolves devices from multiple JOB worker threads",
          "[usb][udev][edge][hardware][threading]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbUdev udev;

    const uint8_t busNumber = fixture.busNumber();
    const uint8_t deviceAddress = fixture.deviceAddress();

    const auto expectedPath = udev.sysfsPath(busNumber, deviceAddress);

    if (!expectedPath)
        SKIP("udev could not resolve the selected libusb device");

    constexpr std::size_t WorkerCount = 8;
    constexpr std::size_t TaskCount = 256;

    job::threads::JobFifoCtx ctx{WorkerCount};

    REQUIRE(ctx.pool);
    REQUIRE(ctx.pool->workerCount() == WorkerCount);

    std::vector<std::future<bool>> futures;
    futures.reserve(TaskCount);

    for (std::size_t task = 0; task < TaskCount; ++task) {
        futures.emplace_back(ctx.pool->submit([&udev, busNumber, deviceAddress, expectedPath] {
            const auto path = udev.sysfsPath(busNumber, deviceAddress);

            return path && *path == *expectedPath;
        }));
    }

    for (auto &future : futures) {
        REQUIRE(future.valid());
        REQUIRE(future.get());
    }
}

#endif // JOB_CI_BUILD

} // namespace job::usb::tests