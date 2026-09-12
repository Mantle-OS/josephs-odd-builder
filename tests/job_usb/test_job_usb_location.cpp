#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <job_usb_location.h>

namespace job::usb::tests {

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobUsbLocation describes the current USB topology location",
          "[usb][location][usage]")
{
    JobUsbLocation::PortPath portPath{2, 4, 1};

    JobUsbLocation location{
        3,
        17,
        portPath,
        "/sys/bus/usb/devices/3-2.4.1"
    };

    REQUIRE(location.busNumber() == 3);
    REQUIRE(location.deviceAddress() == 17);

    REQUIRE(location.portPath().size() == 3);
    REQUIRE(location.portPath()[0] == 2);
    REQUIRE(location.portPath()[1] == 4);
    REQUIRE(location.portPath()[2] == 1);

    REQUIRE(location.path());
    REQUIRE(location.pathView() == "/sys/bus/usb/devices/3-2.4.1");
}

TEST_CASE("JobUsbLocation fields can be updated after construction",
          "[usb][location][usage]")
{
    JobUsbLocation location;

    location.setBusNumber(5);
    location.setDeviceAddress(23);
    location.setPortPath(JobUsbLocation::PortPath{1, 7, 3});
    location.setPath("/sys/bus/usb/devices/5-1.7.3");

    REQUIRE(location.busNumber() == 5);
    REQUIRE(location.deviceAddress() == 23);

    REQUIRE(location.portPath().size() == 3);
    REQUIRE(location.portPath()[0] == 1);
    REQUIRE(location.portPath()[1] == 7);
    REQUIRE(location.portPath()[2] == 3);

    REQUIRE(location.path());
    REQUIRE(location.pathView() == "/sys/bus/usb/devices/5-1.7.3");
}

TEST_CASE("JobUsbLocation factories construct populated locations",
          "[usb][location][factory]")
{
    auto shared = JobUsbLocation::createShared(
        2,
        9,
        JobUsbLocation::PortPath{3, 2},
        "/sys/bus/usb/devices/2-3.2"
        );

    REQUIRE(shared);
    REQUIRE(shared->busNumber() == 2);
    REQUIRE(shared->deviceAddress() == 9);
    REQUIRE(shared->portPath() == JobUsbLocation::PortPath{3, 2});
    REQUIRE(shared->pathView() == "/sys/bus/usb/devices/2-3.2");

    auto unique = JobUsbLocation::createUniq(
        7,
        31,
        JobUsbLocation::PortPath{6},
        "/sys/bus/usb/devices/7-6"
        );

    REQUIRE(unique);
    REQUIRE(unique->busNumber() == 7);
    REQUIRE(unique->deviceAddress() == 31);
    REQUIRE(unique->portPath() == JobUsbLocation::PortPath{6});
    REQUIRE(unique->pathView() == "/sys/bus/usb/devices/7-6");
}

TEST_CASE("JobUsbLocation survives JOB serialization round trips",
          "[usb][location][serialization]")
{
    JobUsbLocation source{
        4,
        12,
        JobUsbLocation::PortPath{2, 5, 3, 1},
        "/sys/bus/usb/devices/4-2.5.3.1"
    };

    SECTION("JSON")
    {
        std::string data;
        REQUIRE(source.toJobJson(data));

        JobUsbLocation restored;
        REQUIRE(restored.fromJobJson(data));

        REQUIRE(restored.busNumber() == source.busNumber());
        REQUIRE(restored.deviceAddress() == source.deviceAddress());
        REQUIRE(restored.portPath() == source.portPath());
        REQUIRE(restored.path() == source.path());
    }

    SECTION("YAML")
    {
        std::string data;
        REQUIRE(source.toJobYaml(data));

        JobUsbLocation restored;
        REQUIRE(restored.fromJobYaml(data));

        REQUIRE(restored.busNumber() == source.busNumber());
        REQUIRE(restored.deviceAddress() == source.deviceAddress());
        REQUIRE(restored.portPath() == source.portPath());
        REQUIRE(restored.path() == source.path());
    }

    SECTION("binary")
    {
        std::vector<std::uint8_t> data;
        REQUIRE(source.toBinary(data));

        JobUsbLocation restored;
        REQUIRE(restored.fromBinary(data));

        REQUIRE(restored.busNumber() == source.busNumber());
        REQUIRE(restored.deviceAddress() == source.deviceAddress());
        REQUIRE(restored.portPath() == source.portPath());
        REQUIRE(restored.path() == source.path());
    }
}

// =============================================================================
// Block 2: Edge cases
// =============================================================================

TEST_CASE("JobUsbLocation default construction represents an unknown location",
          "[usb][location][edge]")
{
    JobUsbLocation location;

    REQUIRE(location.busNumber() == 0);
    REQUIRE(location.deviceAddress() == 0);
    REQUIRE(location.portPath().empty());
    REQUIRE_FALSE(location.path());
    REQUIRE(location.pathView().empty());
}

TEST_CASE("JobUsbLocation supports the maximum USB topology depth",
          "[usb][location][edge][topology]")
{
    JobUsbLocation::PortPath portPath{
        1,
        2,
        3,
        4,
        5,
        6,
        7
    };

    REQUIRE(portPath.size() == 7);
    REQUIRE(portPath.size() == portPath.capacity());

    JobUsbLocation location{
        1,
        42,
        portPath,
        "/sys/bus/usb/devices/deep-device"
    };

    REQUIRE(location.portPath().size() == 7);

    for (std::size_t i = 0; i < location.portPath().size(); ++i)
        REQUIRE(location.portPath()[i] == static_cast<uint8_t>(i + 1));
}

TEST_CASE("JobUsbLocation distinguishes a missing path from an empty path",
          "[usb][location][edge][path]")
{
    JobUsbLocation missing;
    REQUIRE_FALSE(missing.path());
    REQUIRE(missing.pathView().empty());

    JobUsbLocation empty;
    empty.setPath(std::string{});

    REQUIRE(empty.path());
    REQUIRE(empty.path()->empty());
    REQUIRE(empty.pathView().empty());
}

TEST_CASE("JobUsbLocation copies and moves complete location state",
          "[usb][location][edge][lifetime]")
{
    JobUsbLocation source{
        8,
        64,
        JobUsbLocation::PortPath{4, 2, 6},
        "/sys/bus/usb/devices/8-4.2.6"
    };

    JobUsbLocation copied{source};

    REQUIRE(copied.busNumber() == source.busNumber());
    REQUIRE(copied.deviceAddress() == source.deviceAddress());
    REQUIRE(copied.portPath() == source.portPath());
    REQUIRE(copied.path() == source.path());

    JobUsbLocation moved{std::move(copied)};

    REQUIRE(moved.busNumber() == 8);
    REQUIRE(moved.deviceAddress() == 64);
    REQUIRE(moved.portPath() == JobUsbLocation::PortPath{4, 2, 6});
    REQUIRE(moved.pathView() == "/sys/bus/usb/devices/8-4.2.6");
}

} // namespace job::usb::tests