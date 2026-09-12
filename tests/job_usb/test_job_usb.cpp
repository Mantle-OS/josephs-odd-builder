#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#include <libusb-1.0/libusb.h>

#include <job_usb.h>
#include <job_usb_id_database.h>
#include <job_usb_udev.h>

#include "../test_spin_till.h"

namespace job::usb::tests {

//////////////////////////////////////////////////////////
// Block 1: Construction / factories
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb constructs stopped by default",
          "[job_usb][facade][construction]")
{
    JobUsb usb;

    REQUIRE_FALSE(usb.initialized());
    REQUIRE(usb.empty());
    REQUIRE(usb.deviceCount() == 0);
    REQUIRE(usb.lastError().empty());
}

TEST_CASE("JobUsb factories create stopped facades",
          "[job_usb][facade][factory]")
{
    const auto shared = JobUsb::createShared();
    const auto unique = JobUsb::createUniq();

    REQUIRE(shared);
    REQUIRE(unique);

    REQUIRE_FALSE(shared->initialized());
    REQUIRE_FALSE(unique->initialized());

    REQUIRE(shared->empty());
    REQUIRE(unique->empty());

    REQUIRE(shared->deviceCount() == 0);
    REQUIRE(unique->deviceCount() == 0);
}

//////////////////////////////////////////////////////////
// Block 2: Pre-start behavior
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb scan requires an initialized libusb context",
          "[job_usb][facade][scan][invalid]")
{
    JobUsb usb;

    REQUIRE_FALSE(usb.scan());

    REQUIRE_FALSE(usb.initialized());
    REQUIRE_FALSE(usb.lastError().empty());
    REQUIRE(usb.empty());
}

TEST_CASE("JobUsb lookups miss on an empty facade",
          "[job_usb][facade][lookup][empty]")
{
    JobUsb usb;

    REQUIRE(usb.deviceByUid(1) == nullptr);
    REQUIRE(usb.deviceByPath("/sys/does/not/exist") == nullptr);
    REQUIRE(usb.deviceByBusAndAddr(1, 1) == nullptr);
}

TEST_CASE("JobUsb stop is safe before start",
          "[job_usb][facade][lifecycle]")
{
    JobUsb usb;

    usb.stop();
    usb.stop();

    REQUIRE_FALSE(usb.initialized());
    REQUIRE(usb.empty());
    REQUIRE(usb.deviceCount() == 0);
}

#ifndef JOB_CI_BUILD

//////////////////////////////////////////////////////////
// Block 3: Full subsystem startup / scan
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb starts scans and stops the USB subsystem",
          "[job_usb][facade][local][integration]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE_FALSE(usb.initialized());

    REQUIRE(usb.start());

    REQUIRE(usb.initialized());
    REQUIRE(usb.lastError().empty());

    REQUIRE(usb.deviceCount() > 0);
    REQUIRE_FALSE(usb.empty());

    const std::size_t initialCount = usb.deviceCount();

    REQUIRE(usb.scan());

    REQUIRE(usb.initialized());
    REQUIRE(usb.lastError().empty());
    REQUIRE(usb.deviceCount() == initialCount);

    usb.stop();

    REQUIRE_FALSE(usb.initialized());
    REQUIRE(usb.empty());
    REQUIRE(usb.deviceCount() == 0);
}

TEST_CASE("JobUsb start is idempotent",
          "[job_usb][facade][local][lifecycle]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());
    REQUIRE(usb.initialized());

    const std::size_t count = usb.deviceCount();

    REQUIRE(usb.start());

    REQUIRE(usb.initialized());
    REQUIRE(usb.deviceCount() == count);

    usb.stop();

    REQUIRE_FALSE(usb.initialized());
}

TEST_CASE("JobUsb can restart after stop",
          "[job_usb][facade][local][lifecycle][restart]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());
    REQUIRE(usb.initialized());
    REQUIRE(usb.deviceCount() > 0);

    usb.stop();

    REQUIRE_FALSE(usb.initialized());
    REQUIRE(usb.empty());

    REQUIRE(usb.start());

    REQUIRE(usb.initialized());
    REQUIRE(usb.deviceCount() > 0);

    usb.stop();
}

TEST_CASE("JobUsb autoStart initializes the subsystem",
          "[job_usb][facade][local][construction][autostart]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb{true};

    REQUIRE(usb.initialized());
    REQUIRE(usb.lastError().empty());
    REQUIRE(usb.deviceCount() > 0);

    usb.stop();
}

//////////////////////////////////////////////////////////
// Block 4: Database facade shortcuts
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb exposes usb.ids lookup shortcuts",
          "[job_usb][facade][local][database]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());

    REQUIRE_FALSE(usb.vendorName(0x046d).empty());
    REQUIRE_FALSE(usb.productName(0x046d, 0xc534).empty());

    REQUIRE(usb.vendorName(0xffff).empty());
    REQUIRE(usb.productName(0xffff, 0xffff).empty());

    usb.stop();
}

//////////////////////////////////////////////////////////
// Block 5: Device lookup / UID preservation
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb finds devices by bus address and uid",
          "[job_usb][facade][local][lookup]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());
    REQUIRE(usb.deviceCount() > 0);

    libusb_context *context = nullptr;

    REQUIRE(libusb_init(&context) == LIBUSB_SUCCESS);
    REQUIRE(context != nullptr);

    libusb_device **nativeDevices = nullptr;

    const ssize_t nativeCount =
        libusb_get_device_list(context, &nativeDevices);

    REQUIRE(nativeCount > 0);
    REQUIRE(nativeDevices != nullptr);

    JobUsbDevice *found = nullptr;

    for (ssize_t i = 0; i < nativeCount; ++i) {
        const uint8_t bus =
            libusb_get_bus_number(nativeDevices[i]);

        const uint8_t address =
            libusb_get_device_address(nativeDevices[i]);

        found = usb.deviceByBusAndAddr(bus, address);

        if (found)
            break;
    }

    REQUIRE(found != nullptr);
    REQUIRE(found->uid() != 0);

    const uint64_t uid = found->uid();

    REQUIRE(usb.deviceByUid(uid) == found);

    libusb_free_device_list(nativeDevices, 1);
    libusb_exit(context);

    //////////////////////////////////////////////////////////
    // Rescan must preserve runtime identity
    //////////////////////////////////////////////////////////

    REQUIRE(usb.scan());

    JobUsbDevice *rescanned = usb.deviceByUid(uid);

    REQUIRE(rescanned != nullptr);
    REQUIRE(rescanned->uid() == uid);

    usb.stop();
}

TEST_CASE("JobUsb finds a device by sysfs path",
          "[job_usb][facade][local][lookup][path]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());

    libusb_context *context = nullptr;

    REQUIRE(libusb_init(&context) == LIBUSB_SUCCESS);
    REQUIRE(context != nullptr);

    libusb_device **nativeDevices = nullptr;

    const ssize_t nativeCount =
        libusb_get_device_list(context, &nativeDevices);

    REQUIRE(nativeCount > 0);
    REQUIRE(nativeDevices != nullptr);

    JobUsbUdev udev;

    REQUIRE(udev.valid());

    std::string path;

    for (ssize_t i = 0; i < nativeCount; ++i) {
        const uint8_t bus =
            libusb_get_bus_number(nativeDevices[i]);

        const uint8_t address =
            libusb_get_device_address(nativeDevices[i]);

        const auto candidate = udev.sysfsPath(bus, address);

        if (!candidate)
            continue;

        if (usb.deviceByPath(*candidate)) {
            path = *candidate;
            break;
        }
    }

    REQUIRE_FALSE(path.empty());

    JobUsbDevice *device = usb.deviceByPath(path);

    REQUIRE(device != nullptr);
    REQUIRE(device->location().path());
    REQUIRE(*device->location().path() == path);

    libusb_free_device_list(nativeDevices, 1);
    libusb_exit(context);

    usb.stop();
}

//////////////////////////////////////////////////////////
// Block 6: Public hotplug signal
//////////////////////////////////////////////////////////

TEST_CASE("JobUsb rescans and emits deviceChanged for real USB hotplug",
          "[job_usb][facade][local][integration][signal]")
{
    auto &database = JobUsbIdDatabase::instance();
    database.clear();

    JobUsb usb;

    REQUIRE(usb.start());
    REQUIRE(usb.initialized());

    const std::size_t initialCount = usb.deviceCount();

    std::atomic_bool received{false};

    JobUsbMonitorAction receivedAction =
        JobUsbMonitorAction::Unknown;

    std::string receivedPath;

    const auto connection =
        usb.deviceChanged.connect(
            [&](JobUsbMonitorAction action, std::string path) {
                receivedAction = action;
                receivedPath = std::move(path);

                received.store(true, std::memory_order_release);
            });

    REQUIRE(connection);

    std::cout << '\n'
              << "============================================================\n"
              << " JobUsb facade hardware test\n"
              << " Plug in or unplug a USB device within 60 seconds...\n"
              << "============================================================\n"
              << std::flush;

    REQUIRE(spin_until(
        [&] {
            return received.load(std::memory_order_acquire);
        },
        std::chrono::seconds(60)));

    REQUIRE(receivedAction != JobUsbMonitorAction::Unknown);
    REQUIRE_FALSE(receivedPath.empty());

    /*
     * onDeviceChanged() rescans before publishing deviceChanged, so by the
     * time this test observes the signal the facade snapshot is already the
     * post-event snapshot.
     */
    REQUIRE(usb.initialized());

    std::cout << "Received facade USB event: "
              << receivedPath
              << "\nDevice count: "
              << initialCount
              << " -> "
              << usb.deviceCount()
              << '\n';

    usb.stop();

    REQUIRE_FALSE(usb.initialized());
}

#endif

} // namespace job::usb::tests