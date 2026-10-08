#include "job_input_linux_discovery.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <job_logger.h>
#include <job_string_utils.h>

#include <job_dir.h>
#include <job_file.h>
#include <job_syslink.h>

namespace job::input {

JobInputLinuxDiscovery::DevicesResults JobInputLinuxDiscovery::discover(const fs::path &inputPath)
{
    DevicesResults results;

    io::JobDir inputDir(inputPath);
    if (!inputDir.isDirectory()) {
        JOB_LOG_WARN("[JobInputLinuxDiscovery] Input path is not a directory: {}", inputPath.string());
        return results;
    }

    const PathList inputEntries = inputDir.entries();

    if (inputEntries.empty()) {
        JOB_LOG_WARN("[JobInputLinuxDiscovery] No input devices found under {}. /dev may not be mounted or input devices may not be available.", inputPath.string());
        return results;
    }

    const fs::path byIdPath = inputPath / "by-id";
    const fs::path byPathPath = inputPath / "by-path";

    for (const fs::path &path : inputEntries) {
        if (!isEventDevice(path))
            continue;

        auto device = JobInputLinuxDevice::createShared(path);
        if (!device->openDevice()) {
#ifndef NDEBUG
            JOB_LOG_DEBUG("[JobInputLinuxDiscovery] Could not open device {}", path);
#endif
            continue;
        }

        JobInputLinuxDiscoveryResults result;

        result.eventPath = path;
        result.byIdPaths = aliasesForEvent(byIdPath, path);
        result.byPathPaths = aliasesForEvent(byPathPath, path);
        result.mousePath = legacyMouseForEvent(path, inputEntries);
        result.sysfs = sysfsInfoForEvent(path);
        result.device = std::move(device);

        results.emplace_back(std::move(result));
    }

    std::ranges::sort(results, [](const JobInputLinuxDiscoveryResults &lhs, const JobInputLinuxDiscoveryResults &rhs) {
        const auto lhsNumber = eventNumber(lhs.eventPath);
        const auto rhsNumber = eventNumber(rhs.eventPath);

        if (!lhsNumber)
            return false;

        if (!rhsNumber)
            return true;

        return *lhsNumber < *rhsNumber;
    });

    return results;
}

bool JobInputLinuxDiscovery::hasMice() noexcept
{
    return isMiceDevice(kMicePath);
}

bool JobInputLinuxDiscovery::hasUInput() noexcept
{
    return isUInput(kUInputPath);
}

bool JobInputLinuxDiscovery::isEventDevice(const fs::path &path)
{
    return eventNumber(path).has_value();
}

bool JobInputLinuxDiscovery::isLegacyMouseDevice(const fs::path &path)
{
    return mouseNumber(path).has_value();
}

bool JobInputLinuxDiscovery::isMiceDevice(const fs::path &path)
{
    if (path.filename() != "mice")
        return false;

    std::error_code ec;
    return fs::exists(path, ec) && !ec;
}

bool JobInputLinuxDiscovery::isUInput(const fs::path &path)
{
    if (path.filename() != "uinput")
        return false;

    std::error_code ec;
    return fs::exists(path, ec) && !ec;
}

std::optional<std::uint32_t> JobInputLinuxDiscovery::eventNumber(const fs::path &path)
{
    const std::string filename = path.filename().string();
    constexpr std::string_view prefix{"event"};

    if (!filename.starts_with(prefix) || filename.size() == prefix.size())
        return std::nullopt;

    const std::string_view numberText{filename.data() + prefix.size(), filename.size() - prefix.size()};
    if (!std::ranges::all_of(numberText, [](unsigned char c) { return std::isdigit(c) != 0; }))
        return std::nullopt;

    return core::JobStringUtils::toInteger<std::uint32_t>(numberText);
}

std::optional<std::uint32_t> JobInputLinuxDiscovery::mouseNumber(const fs::path &path)
{
    const std::string filename = path.filename().string();
    constexpr std::string_view prefix{"mouse"};

    if (!filename.starts_with(prefix) || filename.size() == prefix.size())
        return std::nullopt;

    const std::string_view numberText{filename.data() + prefix.size(), filename.size() - prefix.size()};
    if (!std::ranges::all_of(numberText, [](unsigned char c) { return std::isdigit(c) != 0; }))
        return std::nullopt;

    return core::JobStringUtils::toInteger<std::uint32_t>(numberText);
}



JobInputLinuxDiscovery::PathList JobInputLinuxDiscovery::aliasesForEvent(const fs::path &directory, const fs::path &eventPath)
{
    PathList aliases;

    io::JobDir aliasDir(directory);
    if (!aliasDir.isDirectory())
        return aliases;

    std::error_code ec;
    const fs::path canonicalEventPath = fs::canonical(eventPath, ec);

    if (ec)
        return aliases;

    for (const fs::path &path : aliasDir.entries()) {
        if (!io::JobSysLink::isSysLink(path))
            continue;

        if (io::JobSysLink::isDangling(path))
            continue;

        const fs::path target = io::JobSysLink::canonicalTarget(path);
        if (!target.empty() && target == canonicalEventPath)
            aliases.emplace_back(path);
    }

    std::ranges::sort(aliases);

    return aliases;
}

fs::path JobInputLinuxDiscovery::legacyMouseForEvent(const fs::path &eventPath, const PathList &inputEntries)
{
    const fs::path eventSysfsPath = sysfsPathForInput(eventPath);
    if (eventSysfsPath.empty())
        return {};

    const fs::path eventInputPath = eventSysfsPath.parent_path();

    for (const fs::path &path : inputEntries) {
        if (!isLegacyMouseDevice(path))
            continue;

        const fs::path mouseSysfsPath = sysfsPathForInput(path);
        if (mouseSysfsPath.empty())
            continue;

        if (mouseSysfsPath.parent_path() == eventInputPath)
            return path;
    }

    return {};
}

fs::path JobInputLinuxDiscovery::devicePathForInput(const fs::path &inputPath)
{
    if (inputPath.empty())
        return {};

    const fs::path deviceLink = inputPath / "device";

    if (!io::JobSysLink::isSysLink(deviceLink))
        return {};

    return io::JobSysLink::canonicalTarget(deviceLink);
}

JobInputLinuxSysfsInfo JobInputLinuxDiscovery::sysfsInfoForEvent(const fs::path &eventPath)
{
    JobInputLinuxSysfsInfo info;
    info.eventPath = sysfsPathForInput(eventPath);
    if (info.eventPath.empty())
        return info;

    info.inputPath = info.eventPath.parent_path();
    info.devicePath = devicePathForInput(info.inputPath);

    const fs::path searchPath = info.devicePath.empty() ? info.inputPath : info.devicePath;

    info.subsystemPath = nearestSysLinkTarget(searchPath, "subsystem");
    info.driverPath = nearestSysLinkTarget(searchPath, "driver");

    info.modalias = readSysfsValue(searchPath / "modalias");
    info.uevent = readUEvent(searchPath / "uevent");

    return info;
}




fs::path JobInputLinuxDiscovery::sysfsPathForInput(const fs::path &path)
{
    const std::string filename = path.filename().string();
    if (filename.empty())
        return {};

    return io::JobSysLink::canonicalTarget(kSysClassInputPath / filename);
}


fs::path JobInputLinuxDiscovery::nearestSysLinkTarget(const fs::path &startPath, std::string_view name)
{
    fs::path current = startPath;
    while (!current.empty() && current != current.root_path()) {
        const fs::path linkPath = current / name;
        if (io::JobSysLink::isSysLink(linkPath)) {
            const fs::path target = io::JobSysLink::canonicalTarget(linkPath);

            if (!target.empty())
                return target;
        }

        current = current.parent_path();
    }

    return {};
}

std::string JobInputLinuxDiscovery::readSysfsValue(const fs::path &path)
{
    io::JobFile file(path);
    if (!file.openDevice())
        return {};

    std::string value;

    if (file.readAll(value) < 0)
        return {};

    while (!value.empty()           &&
           (value.back() == '\n'    ||
            value.back() == '\r'    ||
            value.back() == ' '     ||
            value.back() == '\t')) {
        value.pop_back();
    }

    return value;
}

JobInputLinuxSysfsInfo::UEventEntries JobInputLinuxDiscovery::readUEvent(const fs::path &path)
{
    JobInputLinuxSysfsInfo::UEventEntries entries;

    const std::string data = readSysfsValue(path);
    if (data.empty())
        return entries;

    std::size_t offset = 0;

    while (offset < data.size()) {
        const std::size_t lineEnd = data.find('\n', offset);
        const std::size_t length = lineEnd == std::string::npos ? data.size() - offset : lineEnd - offset;
        const std::string_view line(data.data() + offset, length);
        if (!line.empty()) {
            const std::size_t separator = line.find('=');
            if (separator != std::string_view::npos && separator != 0) {
                JobInputLinuxUEventEntry entry;
                entry.key.assign(line.substr(0, separator));
                entry.value.assign(line.substr(separator + 1));
                entries.emplace_back(std::move(entry));
            }
        }

        if (lineEnd == std::string::npos)
            break;

        offset = lineEnd + 1;
    }

    return entries;
}

} // namespace job::input