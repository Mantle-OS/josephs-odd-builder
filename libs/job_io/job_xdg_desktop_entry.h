#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <string_view>

#include "job_ini.h"
#include "job_xdg_exec.h"

namespace job::io {

class JobXdgDesktopEntry : public JobIni
{
public:
    using Ptr  = std::shared_ptr<JobXdgDesktopEntry>;
    using WPtr = std::weak_ptr<JobXdgDesktopEntry>;
    using UPtr = std::unique_ptr<JobXdgDesktopEntry>;

    static constexpr std::string_view IniGroup = "Desktop Entry";

    JobXdgDesktopEntry() = default;
    ~JobXdgDesktopEntry() override = default;

    JobXdgDesktopEntry(const JobXdgDesktopEntry &) = default;
    JobXdgDesktopEntry &operator=(const JobXdgDesktopEntry &) = default;
    JobXdgDesktopEntry(JobXdgDesktopEntry &&) noexcept = default;
    JobXdgDesktopEntry &operator=(JobXdgDesktopEntry &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobXdgDesktopEntry, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobXdgDesktopEntry>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobXdgDesktopEntry, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobXdgDesktopEntry>(std::forward<Args>(args)...);
    }

    [[=core::NoSerialize{}]]  std::filesystem::path path;
    [[=core::NoSerialize{}]]  std::string desktopFileId;

    JobXdgEntryType Type{JobXdgEntryType::Unknown};
    std::string Version;

    JobXdgLocalizedString Name;
    JobXdgLocalizedString GenericName;
    JobXdgLocalizedString Comment;
    JobXdgLocalizedString Icon;
    JobXdgLocalizedString Keywords;

    bool NoDisplay{false};
    bool Hidden{false};

    std::vector<std::string> OnlyShowIn;
    std::vector<std::string> NotShowIn;

    bool DBusActivatable{false};

    std::string TryExec;
    std::string Exec;
    std::string Path;

    bool Terminal{false};

    std::vector<std::string> Actions;
    std::vector<std::string> MimeType;
    std::vector<std::string> Categories;
    std::vector<std::string> Implements;

    std::optional<bool> StartupNotify;
    std::string StartupWMClass;

    bool PrefersNonDefaultGPU{false};
    bool SingleMainWindow{false};

    std::string URL;

    std::vector<JobXdgAction> desktopActions;
    std::vector<JobXdgRawEntry> extensionEntries;
    std::vector<JobXdgRawGroup> extensionGroups;
};

} // namespace job::io