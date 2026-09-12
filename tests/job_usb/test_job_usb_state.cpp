#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

#include <libusb-1.0/libusb.h>

#include <job_usb_state.h>

#include "test_job_usb_fixtures.h"

namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobUsbState represents live USB runtime state",
          "[usb][state][usage]")
{
    JobUsbState state;

    REQUIRE(state.nativeDevice() == nullptr);
    REQUIRE_FALSE(state.present());
    REQUIRE_FALSE(state.openedByJob());

    state.setPresent(true);
    state.setOpenedByJob(true);

    REQUIRE(state.present());
    REQUIRE(state.openedByJob());
}

TEST_CASE("JobUsbState factories construct runtime state",
          "[usb][state][factory]")
{
    auto shared = JobUsbState::createShared(nullptr, true);

    REQUIRE(shared);
    REQUIRE(shared->nativeDevice() == nullptr);
    REQUIRE(shared->present());
    REQUIRE_FALSE(shared->openedByJob());

    auto unique = JobUsbState::createUniq(nullptr, false);

    REQUIRE(unique);
    REQUIRE(unique->nativeDevice() == nullptr);
    REQUIRE_FALSE(unique->present());
    REQUIRE_FALSE(unique->openedByJob());
}

TEST_CASE("JobUsbState runtime fields are excluded from JOB serialization",
          "[usb][state][serialization]")
{
    JobUsbState source;

    source.setPresent(true);
    source.setOpenedByJob(true);

    SECTION("JSON")
    {
        std::string data;
        REQUIRE(source.toJobJson(data));

        JobUsbState restored;
        REQUIRE(restored.fromJobJson(data));

        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.present());
        REQUIRE_FALSE(restored.openedByJob());
    }

    SECTION("YAML")
    {
        std::string data;
        REQUIRE(source.toJobYaml(data));

        JobUsbState restored;
        REQUIRE(restored.fromJobYaml(data));

        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.present());
        REQUIRE_FALSE(restored.openedByJob());
    }

    SECTION("binary")
    {
        std::vector<std::uint8_t> data;
        REQUIRE(source.toBinary(data));

        JobUsbState restored;
        REQUIRE(restored.fromBinary(data));

        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.present());
        REQUIRE_FALSE(restored.openedByJob());
    }
}

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbState accepts a null native device",
          "[usb][state][edge]")
{
    JobUsbState state{nullptr, true};

    REQUIRE(state.nativeDevice() == nullptr);
    REQUIRE(state.present());

    state.setNativeDevice(nullptr);

    REQUIRE(state.nativeDevice() == nullptr);
    REQUIRE(state.present());
}

TEST_CASE("JobUsbState copies runtime flags independently",
          "[usb][state][edge][lifetime]")
{
    JobUsbState source;

    source.setPresent(true);
    source.setOpenedByJob(true);

    JobUsbState copied{source};

    REQUIRE(copied.nativeDevice() == nullptr);
    REQUIRE(copied.present());
    REQUIRE(copied.openedByJob());

    copied.setPresent(false);
    copied.setOpenedByJob(false);

    REQUIRE(source.present());
    REQUIRE(source.openedByJob());

    REQUIRE_FALSE(copied.present());
    REQUIRE_FALSE(copied.openedByJob());
}

TEST_CASE("JobUsbState moves runtime state and clears the source",
          "[usb][state][edge][lifetime]")
{
    JobUsbState source;

    source.setPresent(true);
    source.setOpenedByJob(true);

    JobUsbState moved{std::move(source)};

    REQUIRE(moved.nativeDevice() == nullptr);
    REQUIRE(source.nativeDevice() == nullptr);
}

TEST_CASE("JobUsbState copy and move assignment preserve runtime state",
          "[usb][state][edge][lifetime]")
{
    JobUsbState source;

    source.setPresent(true);
    source.setOpenedByJob(true);

    JobUsbState copied;
    copied = source;

    REQUIRE(copied.nativeDevice() == nullptr);
    REQUIRE(copied.present());
    REQUIRE(copied.openedByJob());

    JobUsbState moved;
    moved = std::move(copied);

    REQUIRE(moved.nativeDevice() == nullptr);

    REQUIRE(copied.nativeDevice() == nullptr);
}

#ifndef JOB_CI_BUILD

TEST_CASE("JobUsbState retains a native libusb device after enumeration is released",
          "[usb][state][hardware][lifetime]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    libusb_device *nativeDevice = fixture.device();

    REQUIRE(nativeDevice != nullptr);

    JobUsbState state{nativeDevice};

    REQUIRE(state.nativeDevice() == nativeDevice);
    REQUIRE(state.present());

    // Drop libusb_get_device_list()'s references. JobUsbState must have retained
    // its own reference to the native device.
    fixture.releaseDeviceList();

    REQUIRE(state.nativeDevice() == nativeDevice);

    libusb_device_descriptor descriptor{};
    REQUIRE(libusb_get_device_descriptor(state.nativeDevice(), &descriptor) == LIBUSB_SUCCESS);
}

TEST_CASE("JobUsbState copies retain independent native libusb references",
          "[usb][state][hardware][lifetime]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbState original{fixture.device()};
    JobUsbState copied{original};

    REQUIRE(original.nativeDevice() != nullptr);
    REQUIRE(copied.nativeDevice() == original.nativeDevice());

    fixture.releaseDeviceList();

    // Releasing one JobUsbState reference must not invalidate the copy.
    original.setNativeDevice(nullptr);

    REQUIRE(original.nativeDevice() == nullptr);
    REQUIRE(copied.nativeDevice() != nullptr);

    libusb_device_descriptor descriptor{};
    REQUIRE(libusb_get_device_descriptor(copied.nativeDevice(), &descriptor) == LIBUSB_SUCCESS);
}

TEST_CASE("JobUsbState move transfers native libusb reference ownership",
          "[usb][state][hardware][lifetime]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    JobUsbState source{fixture.device()};
    libusb_device *nativeDevice = source.nativeDevice();

    fixture.releaseDeviceList();

    JobUsbState destination{std::move(source)};

    REQUIRE(source.nativeDevice() == nullptr);
    REQUIRE(destination.nativeDevice() == nativeDevice);

    libusb_device_descriptor descriptor{};
    REQUIRE(libusb_get_device_descriptor(destination.nativeDevice(), &descriptor) == LIBUSB_SUCCESS);
}

#endif // JOB_CI_BUILD

} // namespace job::usb::tests