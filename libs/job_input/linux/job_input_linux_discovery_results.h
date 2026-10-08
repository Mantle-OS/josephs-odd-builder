#pragma once

#include <filesystem>
#include <vector>

#include "job_input_linux_device.h"
#include "job_input_linux_sysfs_info.h"

namespace job::input {
namespace fs = std::filesystem;
struct JobInputLinuxDiscoveryResults
{
    using PathList = std::vector<fs::path>;
    fs::path                    eventPath;
    fs::path                    mousePath;
    PathList                    byIdPaths;
    PathList                    byPathPaths;
    JobInputLinuxSysfsInfo      sysfs;
    JobInputLinuxDevice::Ptr    device;
};

} // namespace job::input

