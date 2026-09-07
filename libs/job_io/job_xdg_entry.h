#pragma once

#include <concepts>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <job_base_obj.h>

#include "job_xdg_exec.h"

namespace job::io {

class JobXdgParser;

class JobXdgEntry : public core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<JobXdgEntry>;
    using WPtr = std::weak_ptr<JobXdgEntry>;
    using UPtr = std::unique_ptr<JobXdgEntry>;
    using Path = std::filesystem::path;

    JobXdgEntry() = default;
    ~JobXdgEntry() override = default;

    JobXdgEntry(const JobXdgEntry &) = default;
    JobXdgEntry &operator=(const JobXdgEntry &) = default;
    JobXdgEntry(JobXdgEntry &&) noexcept = default;
    JobXdgEntry &operator=(JobXdgEntry &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobXdgEntry, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobXdgEntry>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobXdgEntry, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobXdgEntry>(std::forward<Args>(args)...);
    }

    // Source -----------------------------------------------------------------

    [[nodiscard]] const Path &path() const noexcept;
    [[nodiscard]] const std::string &desktopFileId() const noexcept;

    // Desktop Entry ----------------------------------------------------------

    [[nodiscard]] JobXdgEntryType type() const noexcept;
    [[nodiscard]] const std::string &version() const noexcept;

    // Presentation -----------------------------------------------------------

    [[nodiscard]] const JobXdgLocalizedString &name() const noexcept;
    [[nodiscard]] const JobXdgLocalizedString &genericName() const noexcept;
    [[nodiscard]] const JobXdgLocalizedString &comment() const noexcept;
    [[nodiscard]] const JobXdgLocalizedString &icon() const noexcept;
    [[nodiscard]] const JobXdgLocalizedString &keywords() const noexcept;

    // Visibility -------------------------------------------------------------

    [[nodiscard]] bool noDisplay() const noexcept;
    [[nodiscard]] bool hidden() const noexcept;

    [[nodiscard]] const std::vector<std::string> &onlyShowIn() const noexcept;
    [[nodiscard]] const std::vector<std::string> &notShowIn() const noexcept;

    // Application ------------------------------------------------------------

    [[nodiscard]] bool dbusActivatable() const noexcept;

    [[nodiscard]] const std::string &tryExec() const noexcept;
    [[nodiscard]] const std::string &exec() const noexcept;
    [[nodiscard]] const std::string &workingDirectory() const noexcept;

    [[nodiscard]] bool terminal() const noexcept;

    [[nodiscard]] const std::vector<std::string> &actions() const noexcept;
    [[nodiscard]] const std::vector<std::string> &mimeTypes() const noexcept;
    [[nodiscard]] const std::vector<std::string> &categories() const noexcept;
    [[nodiscard]] const std::vector<std::string> &implements() const noexcept;

    [[nodiscard]] std::optional<bool> startupNotify() const noexcept;
    [[nodiscard]] const std::string &startupWMClass() const noexcept;

    [[nodiscard]] bool prefersNonDefaultGPU() const noexcept;
    [[nodiscard]] bool singleMainWindow() const noexcept;

    // Link -------------------------------------------------------------------

    [[nodiscard]] const std::string &url() const noexcept;

    // Desktop Actions --------------------------------------------------------

    [[nodiscard]] const std::vector<JobXdgAction> &desktopActions() const noexcept;

    // Extension / unknown data ----------------------------------------------

    [[nodiscard]] const std::vector<JobXdgRawEntry> &extensionEntries() const noexcept;
    [[nodiscard]] const std::vector<JobXdgRawGroup> &extensionGroups() const noexcept;

private:
    friend class JobXdg;

    Path m_path;
    std::string m_desktopFileId;

    JobXdgEntryType m_type{JobXdgEntryType::Unknown};
    std::string m_version;

    JobXdgLocalizedString m_name;
    JobXdgLocalizedString m_genericName;
    JobXdgLocalizedString m_comment;
    JobXdgLocalizedString m_icon;
    JobXdgLocalizedString m_keywords;

    bool m_noDisplay{false};
    bool m_hidden{false};

    std::vector<std::string> m_onlyShowIn;
    std::vector<std::string> m_notShowIn;

    bool m_dbusActivatable{false};

    std::string m_tryExec;
    std::string m_exec;
    std::string m_workingDirectory;

    bool m_terminal{false};

    std::vector<std::string> m_actions;
    std::vector<std::string> m_mimeTypes;
    std::vector<std::string> m_categories;
    std::vector<std::string> m_implements;

    std::optional<bool> m_startupNotify;
    std::string m_startupWMClass;

    bool m_prefersNonDefaultGPU{false};
    bool m_singleMainWindow{false};

    std::string m_url;

    std::vector<JobXdgAction> m_desktopActions;

    std::vector<JobXdgRawEntry> m_extensionEntries;
    std::vector<JobXdgRawGroup> m_extensionGroups;
};

} // namespace job::io