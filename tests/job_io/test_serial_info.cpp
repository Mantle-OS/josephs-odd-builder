#include <catch2/catch_test_macros.hpp>
#include "serial/serial_info.h"
#include "serial/serial_io.h"
#include <iostream>
#include <regex>

using namespace job::io;

namespace fs = std::filesystem;

TEST_CASE("SerialInfo regex matches typical serial device names", "[serial_info][regex]") {
    std::vector<std::string> validNames = {
        "ttyS0", "ttyUSB1", "ttyACM0", "ttyAMA1",
        "ttyGS2", "ttyMI3", "ttymxc4", "ttyTHS5",
        "ttyO6", "rfcomm0", "ircomm1", "tnt0"
    };

    for (const auto &name : validNames) {
        INFO("Checking: " << name);
        auto matches = std::regex_match(name, std::regex(R"(tty(S|USB|ACM|AMA|GS|MI|mxc|THS|O)\d+|rfcomm\d+|ircomm\d+|tnt\d+)"));
        REQUIRE(matches);
    }

    std::vector<std::string> invalidNames = {"audio0", "video1", "tty_fake", "sda1"};
    for (const auto &name : invalidNames) {
        INFO("Checking invalid: " << name);
        auto matches = std::regex_match(name, std::regex(R"(tty(S|USB|ACM|AMA|GS|MI|mxc|THS|O)\d+|rfcomm\d+|ircomm\d+|tnt\d+)"));
        REQUIRE_FALSE(matches);
    }
}

TEST_CASE("SerialInfo filesystem scan finds expected devices", "[serial_info][filesystem]") {
    // Run scan — this should never throw
    REQUIRE_NOTHROW(serial_info::scanSerialPortsFromFilesystem());

    auto ports = serial_info::scanSerialPortsFromFilesystem();

    INFO("Found " << ports.size() << " potential serial device(s) under /dev");

    // It's valid for there to be zero ports (e.g. on CI or no devices connected),
    // but we still verify that any returned paths look correct.
    REQUIRE(std::all_of(ports.begin(), ports.end(), [](const std::string &p) {
        return p.rfind("/dev/", 0) == 0;  // all must begin with /dev/
    }));

    // Optional sanity: ensure the function didn't return a ridiculous number
    REQUIRE(ports.size() < 1024);

    // Print any found devices for diagnostics
    if (!ports.empty()) {
        std::cout << "[SerialInfo] Detected ports:\n";
        for (const auto &p : ports)
            std::cout << "  " << p << '\n';
    } else {
        std::cout << "[SerialInfo] No serial ports found.\n";
    }
}

TEST_CASE("SerialInfo update_serial_devices() populates device map", "[serial_info][udev]") {
    std::map<std::string, std::unique_ptr<SerialIO>> deviceMap;

    REQUIRE_NOTHROW(serial_info::update_serial_devices(deviceMap));

    INFO("Device count after udev scan: " << deviceMap.size());
    for (const auto &[path, devPtr] : deviceMap) {
        REQUIRE(devPtr);
        REQUIRE(!path.empty());
        REQUIRE(devPtr->location() == path);
    }
}
