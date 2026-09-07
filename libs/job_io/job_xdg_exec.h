#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <job_base_obj.h>

namespace job::io {

enum class JobXdgEntryType : std::uint8_t
{
    Unknown,
    Application,
    Link,
    Directory
};

[[nodiscard]] inline bool fromIniValue(std::string_view value,
                                       JobXdgEntryType &type) noexcept
{
    if (value == "Application") {
        type = JobXdgEntryType::Application;
        return true;
    }

    if (value == "Link") {
        type = JobXdgEntryType::Link;
        return true;
    }

    if (value == "Directory") {
        type = JobXdgEntryType::Directory;
        return true;
    }

    type = JobXdgEntryType::Unknown;
    return false;
}

[[nodiscard]] inline std::string toIniValue(JobXdgEntryType type)
{
    switch (type) {
    case JobXdgEntryType::Application:
        return "Application";
    case JobXdgEntryType::Link:
        return "Link";
    case JobXdgEntryType::Directory:
        return "Directory";
    case JobXdgEntryType::Unknown:
        return {};
    }

    return {};
}

struct JobXdgLocalizedString : public core::BaseObject
{
    std::string value;
    std::map<std::string, std::string> localized;
};

struct JobXdgRawEntry : public core::BaseObject
{
    std::string key;
    std::string value;
};

struct JobXdgAction : public core::BaseObject
{
    std::string id;
    JobXdgLocalizedString name;
    JobXdgLocalizedString icon;
    std::string exec;
    std::vector<JobXdgRawEntry> extensionEntries;
};

struct JobXdgRawGroup : public core::BaseObject
{
    std::string name;
    std::vector<JobXdgRawEntry> entries;
};

struct XdgExec final
{
    using Path = std::filesystem::path;

    std::string program;
    std::vector<std::string> arguments;
    Path workingDirectory;
    bool terminal{false};
};

} // namespace job::io