#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <serial_manager.h>
#include <serial_info.h>

using namespace job::uart;

TEST_CASE("SerialManager initializes and scans devices", "[serial_manager][init]") {
    REQUIRE_NOTHROW(SerialManager());

    SerialManager manager;
    const auto &devices = manager.devices();

    size_t before = devices.size();
    manager.scanDevices();
    size_t after = manager.devices().size();
    INFO("Device count before: " << before << ", after: " << after);
    REQUIRE(after >= before);

    if (!devices.empty()) {
        // Ensure all device paths begin with /dev/
        for (const auto &[path, io] : devices)
            REQUIRE(path.rfind("/dev/", 0) == 0);
    }
}

TEST_CASE("SerialManager currentDevice reflects first detected port", "[serial_manager][current]") {
    SerialManager manager;
    const auto &devices = manager.devices();

    if (!devices.empty()) {
        auto *cur = manager.currentDevice();
        REQUIRE(cur != nullptr);

        const auto &path = cur->location();
        REQUIRE(devices.find(path) != devices.end());

        INFO("Current device: " << path);
    } else {
        SUCCEED("No serial devices attached, skipping currentDevice validation.");
    }
}

TEST_CASE("SerialManager can switch currentDevice", "[serial_manager][switch]") {
    SerialManager manager;
    const auto &devices = manager.devices();

    if (devices.size() > 1) {
        auto it = std::next(devices.begin());
        const auto &targetPath = it->first;

        REQUIRE_NOTHROW(manager.setCurrentDevice(targetPath));

        auto *cur = manager.currentDevice();
        REQUIRE(cur != nullptr);
        REQUIRE(cur->location() == targetPath);
    } else {
        SUCCEED("Only one or zero devices present; skipping switch test.");
    }
}

TEST_CASE("SerialManager responds to rescan (udev simulated)", "[serial_manager][rescan]") {
    SerialManager manager;
    size_t before = manager.devices().size();

    // Force a rescan (same as udev callback)
    REQUIRE_NOTHROW(manager.scanDevices());
    size_t after = manager.devices().size();

    INFO("Device count changed from " << before << " to " << after);

    // The test should confirm the rescan does *not* lose entries
    REQUIRE(after >= before);

    // Optionally, check that pointers are still valid
    for (const auto &[path, dev] : manager.devices()) {
        REQUIRE(dev != nullptr);
        REQUIRE(path.rfind("/dev/", 0) == 0);
    }
}
