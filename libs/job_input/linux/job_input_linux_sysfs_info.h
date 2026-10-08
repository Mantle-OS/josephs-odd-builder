#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace job::input {

namespace fs = std::filesystem;

struct JobInputLinuxUEventEntry
{
    std::string key;
    std::string value;
};

struct JobInputLinuxSysfsInfo
{
    using UEventEntries = std::vector<JobInputLinuxUEventEntry>;

    // /sys/class/input/eventN -> canonical /sys/devices/.../input/inputX/eventN
    fs::path eventPath;

    // Parent input device, usually .../input/inputX
    fs::path inputPath;

    // Nearest useful hardware/topology ancestor discovered walking upward.
    fs::path devicePath;

    // Canonical target of the nearest "subsystem" symlink.
    fs::path subsystemPath;

    // Canonical target of the nearest "driver" symlink.
    fs::path driverPath;

    std::string modalias;
    UEventEntries uevent;

    [[nodiscard]] bool empty() const noexcept
    {
        return eventPath.empty()        &&
               inputPath.empty()        &&
               devicePath.empty()       &&
               subsystemPath.empty()    &&
               driverPath.empty()       &&
               modalias.empty()         &&
               uevent.empty();
    }

    [[nodiscard]] std::string_view ueventValue(std::string_view key) const noexcept
    {
        for (const auto &entry : uevent) {
            if (entry.key == key)
                return entry.value;
        }

        return {};
    }
};

} // namespace job::input