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


TEST_CASE("SerialInfo update_serial_devices() populates device map", "[serial_info][udev]") {
    std::map<std::string, std::unique_ptr<SerialIO>> deviceMap;
    auto loop = std::make_shared<job::threads::JobIoAsyncThread>();
    loop->start();

    REQUIRE_NOTHROW(serial_info::update_serial_devices(deviceMap, loop)); // Pass the loop

    INFO("Device count after udev scan: " << deviceMap.size());
    for (const auto &[path, devPtr] : deviceMap) {
        REQUIRE(devPtr);
        REQUIRE(!path.empty());
        REQUIRE(devPtr->location() == path);
    }

    loop->stop(); // Clean up the loop
}

