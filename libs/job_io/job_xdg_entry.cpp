#include "job_xdg_entry.h"

namespace job::io {

const JobXdgEntry::Path &JobXdgEntry::path() const noexcept
{
    return m_path;
}

const std::string &JobXdgEntry::desktopFileId() const noexcept
{
    return m_desktopFileId;
}

JobXdgEntryType JobXdgEntry::type() const noexcept
{
    return m_type;
}

const std::string &JobXdgEntry::version() const noexcept
{
    return m_version;
}

const JobXdgLocalizedString &JobXdgEntry::name() const noexcept
{
    return m_name;
}

const JobXdgLocalizedString &JobXdgEntry::genericName() const noexcept
{
    return m_genericName;
}

const JobXdgLocalizedString &JobXdgEntry::comment() const noexcept
{
    return m_comment;
}

const JobXdgLocalizedString &JobXdgEntry::icon() const noexcept
{
    return m_icon;
}

const JobXdgLocalizedString &JobXdgEntry::keywords() const noexcept
{
    return m_keywords;
}

bool JobXdgEntry::noDisplay() const noexcept
{
    return m_noDisplay;
}

bool JobXdgEntry::hidden() const noexcept
{
    return m_hidden;
}

const std::vector<std::string> &JobXdgEntry::onlyShowIn() const noexcept
{
    return m_onlyShowIn;
}

const std::vector<std::string> &JobXdgEntry::notShowIn() const noexcept
{
    return m_notShowIn;
}

bool JobXdgEntry::dbusActivatable() const noexcept
{
    return m_dbusActivatable;
}

const std::string &JobXdgEntry::tryExec() const noexcept
{
    return m_tryExec;
}

const std::string &JobXdgEntry::exec() const noexcept
{
    return m_exec;
}

const std::string &JobXdgEntry::workingDirectory() const noexcept
{
    return m_workingDirectory;
}

bool JobXdgEntry::terminal() const noexcept
{
    return m_terminal;
}

const std::vector<std::string> &JobXdgEntry::actions() const noexcept
{
    return m_actions;
}

const std::vector<std::string> &JobXdgEntry::mimeTypes() const noexcept
{
    return m_mimeTypes;
}

const std::vector<std::string> &JobXdgEntry::categories() const noexcept
{
    return m_categories;
}

const std::vector<std::string> &JobXdgEntry::implements() const noexcept
{
    return m_implements;
}

std::optional<bool> JobXdgEntry::startupNotify() const noexcept
{
    return m_startupNotify;
}

const std::string &JobXdgEntry::startupWMClass() const noexcept
{
    return m_startupWMClass;
}

bool JobXdgEntry::prefersNonDefaultGPU() const noexcept
{
    return m_prefersNonDefaultGPU;
}

bool JobXdgEntry::singleMainWindow() const noexcept
{
    return m_singleMainWindow;
}

const std::string &JobXdgEntry::url() const noexcept
{
    return m_url;
}

const std::vector<JobXdgAction> &JobXdgEntry::desktopActions() const noexcept
{
    return m_desktopActions;
}

const std::vector<JobXdgRawEntry> &JobXdgEntry::extensionEntries() const noexcept
{
    return m_extensionEntries;
}

const std::vector<JobXdgRawGroup> &JobXdgEntry::extensionGroups() const noexcept
{
    return m_extensionGroups;
}

} // namespace job::io