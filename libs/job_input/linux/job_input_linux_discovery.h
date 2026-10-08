#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "job_input_linux_discovery_results.h"
#include "job_input_linux_sysfs_info.h"

namespace job::input {

namespace fs = std::filesystem;

class JobInputLinuxDiscovery
{
public:
    using DevicesResults = std::vector<JobInputLinuxDiscoveryResults>;
    using PathList       = std::vector<fs::path>;

    inline static const fs::path kDefaultInputPath{"/dev/input"};
    inline static const fs::path kByIdPath{"/dev/input/by-id"};     // might go away
    inline static const fs::path kByPathPath{"/dev/input/by-path"}; // might go away
    inline static const fs::path kMicePath{"/dev/input/mice"};
    inline static const fs::path kUInputPath{"/dev/uinput"};
    inline static const fs::path kSysClassInputPath{"/sys/class/input"};

    constexpr JobInputLinuxDiscovery() noexcept = delete;
    ~JobInputLinuxDiscovery() = delete;

    [[nodiscard]] static DevicesResults discover(const fs::path &inputPath = kDefaultInputPath);

    [[nodiscard]] static bool hasMice() noexcept;
    [[nodiscard]] static bool hasUInput() noexcept;

private:
    [[nodiscard]] static bool isEventDevice(const fs::path &path);
    [[nodiscard]] static bool isLegacyMouseDevice(const fs::path &path);
    [[nodiscard]] static bool isMiceDevice(const fs::path &path);
    [[nodiscard]] static bool isUInput(const fs::path &path);

    [[nodiscard]] static std::optional<std::uint32_t> eventNumber(const fs::path &path);
    [[nodiscard]] static std::optional<std::uint32_t> mouseNumber(const fs::path &path);

    [[nodiscard]] static PathList aliasesForEvent(const fs::path &directory, const fs::path &eventPath);
    [[nodiscard]] static fs::path legacyMouseForEvent(const fs::path &eventPath, const PathList &inputEntries);

    [[nodiscard]] static fs::path devicePathForInput(const fs::path &inputPath);

    [[nodiscard]] static JobInputLinuxSysfsInfo sysfsInfoForEvent(const fs::path &eventPath);
    [[nodiscard]] static fs::path sysfsPathForInput(const fs::path &path);

    [[nodiscard]] static fs::path topologyDevicePath(const fs::path &inputPath);
    [[nodiscard]] static fs::path nearestSysLinkTarget(const fs::path &startPath, std::string_view name);
    [[nodiscard]] static std::string readSysfsValue(const fs::path &path);
    [[nodiscard]] static JobInputLinuxSysfsInfo::UEventEntries readUEvent(const fs::path &path);
};

} // namespace job::input