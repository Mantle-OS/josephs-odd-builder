#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <libusb-1.0/libusb.h>

#include <job_usb_device.h>

#include "test_job_usb_fixtures.h"

namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobUsbDevice combines descriptor location strings and runtime state",
          "[usb][device][usage]")
{
    JobUsbDevice device{
        42,
        JobUsbDescriptor{
            0x0955,
            0x7321,
            0x0300,
            0x0102,
            0xEF,
            0x02,
            0x01
        },
        JobUsbLocation{
            3,
            17,
            JobUsbLocation::PortPath{2, 4, 1},
            "/sys/bus/usb/devices/3-2.4.1"
        },
        JobUsbStrings{
            "JOB Inc.",
            "SOM-215GS",
            "ABC123456"
        },
        JobUsbState{
            nullptr,
            true
        }
    };

    REQUIRE(device.uid() == 42);

    REQUIRE(device.descriptor().vendorId() == 0x0955);
    REQUIRE(device.descriptor().productId() == 0x7321);
    REQUIRE(device.descriptor().usbSpecVersion() == 0x0300);
    REQUIRE(device.descriptor().deviceVersion() == 0x0102);

    REQUIRE(device.location().busNumber() == 3);
    REQUIRE(device.location().deviceAddress() == 17);
    REQUIRE(device.location().portPath() == JobUsbLocation::PortPath{2, 4, 1});
    REQUIRE(device.location().pathView() == "/sys/bus/usb/devices/3-2.4.1");

    REQUIRE(device.strings().manufacturerView() == "JOB Inc.");
    REQUIRE(device.strings().productView() == "SOM-215GS");
    REQUIRE(device.strings().serialNumberView() == "ABC123456");

    REQUIRE(device.state().present());
    REQUIRE_FALSE(device.state().openedByJob());
    REQUIRE(device.nativeDevice() == nullptr);
}

TEST_CASE("JobUsbDevice exposes mutable component objects",
          "[usb][device][usage]")
{
    JobUsbDevice device;

    device.setUid(100);

    device.descriptor().setVendorId(0x1234);
    device.descriptor().setProductId(0x5678);

    device.location().setBusNumber(4);
    device.location().setDeviceAddress(22);
    device.location().setPortPath(JobUsbLocation::PortPath{1, 3});

    device.strings().setManufacturer("JOB");
    device.strings().setProduct("USB Device");
    device.strings().setSerialNumber("JOB-USB-001");

    device.state().setPresent(true);
    device.state().setOpenedByJob(true);

    REQUIRE(device.uid() == 100);
    REQUIRE(device.descriptor().vendorId() == 0x1234);
    REQUIRE(device.descriptor().productId() == 0x5678);
    REQUIRE(device.location().busNumber() == 4);
    REQUIRE(device.location().deviceAddress() == 22);
    REQUIRE(device.location().portPath() == JobUsbLocation::PortPath{1, 3});
    REQUIRE(device.strings().manufacturerView() == "JOB");
    REQUIRE(device.strings().productView() == "USB Device");
    REQUIRE(device.strings().serialNumberView() == "JOB-USB-001");
    REQUIRE(device.state().present());
    REQUIRE(device.state().openedByJob());
}

TEST_CASE("JobUsbDevice component objects can be replaced",
          "[usb][device][usage]")
{
    JobUsbDevice device;

    device.setDescriptor(JobUsbDescriptor{
        0x1D6B,
        0x0002,
        0x0200,
        0x0601,
        0x09,
        0x00,
        0x01
    });

    device.setLocation(JobUsbLocation{
        1,
        2,
        JobUsbLocation::PortPath{5},
        "/sys/bus/usb/devices/1-5"
    });

    device.setStrings(JobUsbStrings{
        "Linux",
        "Root Hub",
        std::nullopt
    });

    device.setState(JobUsbState{
        nullptr,
        true
    });

    REQUIRE(device.descriptor().vendorId() == 0x1D6B);
    REQUIRE(device.descriptor().productId() == 0x0002);

    REQUIRE(device.location().busNumber() == 1);
    REQUIRE(device.location().deviceAddress() == 2);
    REQUIRE(device.location().portPath() == JobUsbLocation::PortPath{5});

    REQUIRE(device.strings().manufacturerView() == "Linux");
    REQUIRE(device.strings().productView() == "Root Hub");
    REQUIRE_FALSE(device.strings().serialNumber());

    REQUIRE(device.state().present());
}

TEST_CASE("JobUsbDevice factories construct complete USB devices",
          "[usb][device][factory]")
{
    auto shared = JobUsbDevice::createShared(
        10,
        JobUsbDescriptor{
            0x0403,
            0x6001,
            0x0200,
            0x0600,
            0x00,
            0x00,
            0x00
        },
        JobUsbLocation{
            2,
            7,
            JobUsbLocation::PortPath{3},
            "/sys/bus/usb/devices/2-3"
        },
        JobUsbStrings{
            "FTDI",
            "USB Serial",
            "FT0001"
        },
        JobUsbState{
            nullptr,
            true
        }
        );

    REQUIRE(shared);
    REQUIRE(shared->uid() == 10);
    REQUIRE(shared->descriptor().vendorId() == 0x0403);
    REQUIRE(shared->descriptor().productId() == 0x6001);
    REQUIRE(shared->location().busNumber() == 2);
    REQUIRE(shared->strings().serialNumberView() == "FT0001");
    REQUIRE(shared->state().present());

    auto unique = JobUsbDevice::createUniq();

    REQUIRE(unique);
    REQUIRE(unique->uid() == 0);
    REQUIRE(unique->nativeDevice() == nullptr);
}

TEST_CASE("JobUsbDevice survives JOB serialization round trips",
          "[usb][device][serialization]")
{
    JobUsbDevice source{
        987654,
        JobUsbDescriptor{
            0x0955,
            0x7321,
            0x0300,
            0x0102,
            0xEF,
            0x02,
            0x01
        },
        JobUsbLocation{
            6,
            33,
            JobUsbLocation::PortPath{2, 5, 1},
            "/sys/bus/usb/devices/6-2.5.1"
        },
        JobUsbStrings{
            "JOB Inc.",
            "SOM-35D1F",
            "SERIAL-987"
        },
        JobUsbState{
            nullptr,
            true
        }
    };

    source.state().setOpenedByJob(true);

    SECTION("JSON")
    {
        std::string data;
        REQUIRE(source.toJobJson(data));

        JobUsbDevice restored;
        REQUIRE(restored.fromJobJson(data));

        REQUIRE(restored.descriptor().vendorId() == source.descriptor().vendorId());
        REQUIRE(restored.descriptor().productId() == source.descriptor().productId());
        REQUIRE(restored.descriptor().usbSpecVersion() == source.descriptor().usbSpecVersion());
        REQUIRE(restored.descriptor().deviceVersion() == source.descriptor().deviceVersion());

        REQUIRE(restored.location().busNumber() == source.location().busNumber());
        REQUIRE(restored.location().deviceAddress() == source.location().deviceAddress());
        REQUIRE(restored.location().portPath() == source.location().portPath());
        REQUIRE(restored.location().path() == source.location().path());

        REQUIRE(restored.strings().manufacturer() == source.strings().manufacturer());
        REQUIRE(restored.strings().product() == source.strings().product());
        REQUIRE(restored.strings().serialNumber() == source.strings().serialNumber());

        REQUIRE(restored.uid() == 0);
        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.state().present());
        REQUIRE_FALSE(restored.state().openedByJob());
    }

    SECTION("YAML")
    {
        std::string data;
        REQUIRE(source.toJobYaml(data));

        JobUsbDevice restored;
        REQUIRE(restored.fromJobYaml(data));

        REQUIRE(restored.descriptor().vendorId() == source.descriptor().vendorId());
        REQUIRE(restored.descriptor().productId() == source.descriptor().productId());

        REQUIRE(restored.location().busNumber() == source.location().busNumber());
        REQUIRE(restored.location().deviceAddress() == source.location().deviceAddress());
        REQUIRE(restored.location().portPath() == source.location().portPath());
        REQUIRE(restored.location().path() == source.location().path());

        REQUIRE(restored.strings().manufacturer() == source.strings().manufacturer());
        REQUIRE(restored.strings().product() == source.strings().product());
        REQUIRE(restored.strings().serialNumber() == source.strings().serialNumber());

        REQUIRE(restored.uid() == 0);
        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.state().present());
        REQUIRE_FALSE(restored.state().openedByJob());
    }

    SECTION("binary")
    {
        std::vector<std::uint8_t> data;
        REQUIRE(source.toBinary(data));

        JobUsbDevice restored;
        REQUIRE(restored.fromBinary(data));

        REQUIRE(restored.descriptor().vendorId() == source.descriptor().vendorId());
        REQUIRE(restored.descriptor().productId() == source.descriptor().productId());

        REQUIRE(restored.location().busNumber() == source.location().busNumber());
        REQUIRE(restored.location().deviceAddress() == source.location().deviceAddress());
        REQUIRE(restored.location().portPath() == source.location().portPath());
        REQUIRE(restored.location().path() == source.location().path());

        REQUIRE(restored.strings().manufacturer() == source.strings().manufacturer());
        REQUIRE(restored.strings().product() == source.strings().product());
        REQUIRE(restored.strings().serialNumber() == source.strings().serialNumber());

        REQUIRE(restored.uid() == 0);
        REQUIRE(restored.nativeDevice() == nullptr);
        REQUIRE_FALSE(restored.state().present());
        REQUIRE_FALSE(restored.state().openedByJob());
    }
}

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbDevice default construction represents an empty USB device",
          "[usb][device][edge]")
{
    JobUsbDevice device;

    REQUIRE(device.uid() == 0);

    REQUIRE(device.descriptor().vendorId() == 0);
    REQUIRE(device.descriptor().productId() == 0);

    REQUIRE(device.location().busNumber() == 0);
    REQUIRE(device.location().deviceAddress() == 0);
    REQUIRE(device.location().portPath().empty());
    REQUIRE_FALSE(device.location().path());

    REQUIRE_FALSE(device.strings().manufacturer());
    REQUIRE_FALSE(device.strings().product());
    REQUIRE_FALSE(device.strings().serialNumber());

    REQUIRE(device.nativeDevice() == nullptr);
    REQUIRE_FALSE(device.state().present());
    REQUIRE_FALSE(device.state().openedByJob());
}

TEST_CASE("JobUsbDevice UID is runtime identity and is not persistent",
          "[usb][device][edge][serialization]")
{
    JobUsbDevice source;
    source.setUid(UINT64_MAX);

    std::string data;
    REQUIRE(source.toJobJson(data));

    JobUsbDevice restored;
    restored.setUid(123);

    REQUIRE(restored.fromJobJson(data));

    REQUIRE(source.uid() == UINT64_MAX);
    REQUIRE(restored.uid() == 123);
}

#ifndef JOB_CI_BUILD

TEST_CASE("JobUsbDevice retains its native libusb device through JobUsbState",
          "[usb][device][hardware][lifetime]")
{
    LibUsbDeviceFixture fixture;

    if (!fixture.initialized())
        SKIP("libusb could not be initialized");

    if (!fixture.hasDevice())
        SKIP("no USB devices are available");

    libusb_device *nativeDevice = fixture.device();

    JobUsbDevice device{
        1,
        JobUsbDescriptor{},
        JobUsbLocation{},
        JobUsbStrings{},
        JobUsbState{
            nativeDevice,
            true
        }
    };

    REQUIRE(device.nativeDevice() == nativeDevice);
    REQUIRE(device.state().nativeDevice() == nativeDevice);

    // Release the enumeration-owned libusb reference. JobUsbState must keep
    // the native device alive for the lifetime of JobUsbDevice.
    fixture.releaseDeviceList();

    REQUIRE(device.nativeDevice() == nativeDevice);

    libusb_device_descriptor descriptor{};
    REQUIRE(libusb_get_device_descriptor(device.nativeDevice(), &descriptor) == LIBUSB_SUCCESS);
}

TEST_CASE("JobUsbDevice loads expected values from JOB JSON file",
          "[usb][device][json][file]")
{
    JobUsbDevice device;

    REQUIRE(device.loadFromJobJsonFile(JOB_USB_TEST_DEVICE_JSON_FILE));

    REQUIRE(device.descriptor().vendorId() == 2389);
    REQUIRE(device.descriptor().productId() == 29473);
    REQUIRE(device.descriptor().usbSpecVersion() == 768);
    REQUIRE(device.descriptor().deviceVersion() == 258);
    REQUIRE(device.descriptor().deviceClass() == 239);
    REQUIRE(device.descriptor().subClass() == 2);
    REQUIRE(device.descriptor().protocol() == 1);

    REQUIRE(device.location().portPath() == JobUsbLocation::PortPath{2, 4, 1});
    REQUIRE(device.location().pathView() == "/sys/bus/usb/devices/3-2.4.1");
    REQUIRE(device.location().busNumber() == 3);
    REQUIRE(device.location().deviceAddress() == 17);

    REQUIRE(device.strings().manufacturerView() == "JOB Inc.");
    REQUIRE(device.strings().productView() == "SOM-215GS");
    REQUIRE(device.strings().serialNumberView() == "ABC123456");

    REQUIRE(device.uid() == 0);
    REQUIRE(device.nativeDevice() == nullptr);
    REQUIRE_FALSE(device.state().present());
    REQUIRE_FALSE(device.state().openedByJob());
}

TEST_CASE("JobUsbDevice loads expected values from JOB YAML file",
          "[usb][device][yaml][file]")
{
    JobUsbDevice device;

    REQUIRE(device.loadFromJobYamlFile(JOB_USB_TEST_DEVICE_YAML_FILE));

    REQUIRE(device.descriptor().vendorId() == 2389);
    REQUIRE(device.descriptor().productId() == 29473);
    REQUIRE(device.descriptor().usbSpecVersion() == 768);
    REQUIRE(device.descriptor().deviceVersion() == 258);
    REQUIRE(device.descriptor().deviceClass() == 239);
    REQUIRE(device.descriptor().subClass() == 2);
    REQUIRE(device.descriptor().protocol() == 1);

    REQUIRE(device.location().portPath() == JobUsbLocation::PortPath{2, 4, 1});
    REQUIRE(device.location().pathView() == "/sys/bus/usb/devices/3-2.4.1");
    REQUIRE(device.location().busNumber() == 3);
    REQUIRE(device.location().deviceAddress() == 17);

    REQUIRE(device.strings().manufacturerView() == "JOB Inc.");
    REQUIRE(device.strings().productView() == "SOM-215GS");
    REQUIRE(device.strings().serialNumberView() == "ABC123456");

    REQUIRE(device.uid() == 0);
    REQUIRE(device.nativeDevice() == nullptr);
    REQUIRE_FALSE(device.state().present());
    REQUIRE_FALSE(device.state().openedByJob());
}

#endif // JOB_CI_BUILD

TEST_CASE("JobUsbDevice reflected connection propagates persistent device state",
          "[usb][device][signal][connection]")
{
    auto source = JobUsbDevice::createShared();
    auto destination = JobUsbDevice::createShared();

    REQUIRE(source);
    REQUIRE(destination);

    source->setUid(100);
    destination->setUid(200);

    source->state().setPresent(true);
    source->state().setOpenedByJob(true);

    destination->state().setPresent(false);
    destination->state().setOpenedByJob(false);

    auto connection = core::connect<&JobUsbDevice::updated,
                                    &JobUsbDevice::handleUpdated>(*source, *destination);

    REQUIRE(connection);
    REQUIRE(connection.connected());
    REQUIRE(connection.id() != 0);

    REQUIRE(source->updated.connectionCount() == 1);
    REQUIRE(destination->connectionCount() == 1);
    REQUIRE_FALSE(source->updated.empty());

    source->descriptor().setVendorId(0x0955);
    source->descriptor().setProductId(0x7321);

    source->location().setBusNumber(3);
    source->location().setDeviceAddress(17);
    source->location().setPortPath(JobUsbLocation::PortPath{2, 4, 1});
    source->location().setPath("/sys/bus/usb/devices/3-2.4.1");

    source->strings().setManufacturer("JOB Inc.");
    source->strings().setProduct("SOM-215GS");
    source->strings().setSerialNumber("ABC123456");

    source->updated.emit(*source);

    REQUIRE(destination->descriptor().vendorId() == 0x0955);
    REQUIRE(destination->descriptor().productId() == 0x7321);

    REQUIRE(destination->location().busNumber() == 3);
    REQUIRE(destination->location().deviceAddress() == 17);
    REQUIRE(destination->location().portPath() == JobUsbLocation::PortPath{2, 4, 1});
    REQUIRE(destination->location().pathView() == "/sys/bus/usb/devices/3-2.4.1");

    REQUIRE(destination->strings().manufacturerView() == "JOB Inc.");
    REQUIRE(destination->strings().productView() == "SOM-215GS");
    REQUIRE(destination->strings().serialNumberView() == "ABC123456");

    // Runtime identity and runtime backend state belong to the destination.
    REQUIRE(source->uid() == 100);
    REQUIRE(destination->uid() == 200);

    REQUIRE(source->state().present());
    REQUIRE(source->state().openedByJob());

    REQUIRE_FALSE(destination->state().present());
    REQUIRE_FALSE(destination->state().openedByJob());
}

} // namespace job::usb::tests