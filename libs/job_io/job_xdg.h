#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <job_thread_pool.h>

#include "job_xdg_desktop_entry.h"
#include "job_xdg_entry.h"
#include "job_xdg_paths.h"

namespace job::io {

class JobXdg
{
public:
    using EntryList        = std::vector<JobXdgEntry>;
    using DesktopEntryList = std::vector<JobXdgDesktopEntry>;

    using Path     = std::filesystem::path;
    using PathList = std::vector<Path>;

    [[nodiscard]] static JobXdg &instance();

    JobXdg(const JobXdg &) = delete;
    JobXdg &operator=(const JobXdg &) = delete;
    JobXdg(JobXdg &&) = delete;
    JobXdg &operator=(JobXdg &&) = delete;

    [[nodiscard]] const JobXdgPaths &paths() const noexcept;

    [[nodiscard]] const EntryList &entries() const noexcept;
    [[nodiscard]] const DesktopEntryList &desktopEntries() const noexcept;

    [[nodiscard]] const JobXdgEntry *find(std::string_view desktopFileId) const noexcept;
    [[nodiscard]] bool contains(std::string_view desktopFileId) const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] std::optional<XdgExec> exec(std::string_view desktopFileId) const;
    [[nodiscard]] std::optional<XdgExec> exec(const JobXdgEntry &entry) const;

    [[nodiscard]] bool reload();
    [[nodiscard]] bool reloadParallel(job::threads::ThreadPool &pool);

private:
    struct Source final
    {
        Path path;
        std::size_t offset{};
        std::size_t size{};
    };

    using SourceList = std::vector<Source>;

    JobXdg();
    ~JobXdg() = default;

    [[nodiscard]] PathList discoverDesktopFiles() const;

    [[nodiscard]] bool loadSerial(const PathList &files,
                                  EntryList &entries) const;

    [[nodiscard]] bool loadEntry(const Path &path,
                                 JobXdgEntry &entry) const;

    [[nodiscard]] bool parseEntry(const Path &path,
                                  std::string_view source,
                                  JobXdgEntry &entry) const;

    [[nodiscard]] static bool parseBoolean(std::string_view value,
                                           bool &result) noexcept;

    [[nodiscard]] static JobXdgEntryType parseType(std::string_view value) noexcept;

    [[nodiscard]] static std::string unescape(std::string_view value);

    [[nodiscard]] static std::vector<std::string> parseList(std::string_view value);

    [[nodiscard]] bool loadDesktopEntries(const PathList &files,
                                          DesktopEntryList &entries) const;

    [[nodiscard]] bool loadDesktopEntry(const Path &path,
                                        JobXdgDesktopEntry &entry) const;

    [[nodiscard]] bool loadSources(const PathList &files,
                                   std::string &buffer,
                                   SourceList &sources) const;

    [[nodiscard]] bool parseSourcesParallel(job::threads::ThreadPool &pool,
                                            std::string_view buffer,
                                            const SourceList &sources,
                                            EntryList &entries) const;

    [[nodiscard]] static bool splitExec(std::string_view value,
                                        std::vector<std::string> &arguments);

    [[nodiscard]] static bool expandExecField(std::string_view token,
                                              const JobXdgEntry &entry,
                                              std::vector<std::string> &arguments);

    [[nodiscard]] bool buildCatalog(EntryList parsedEntries,
                                    EntryList &catalog) const;

    [[nodiscard]] bool buildDesktopCatalog(DesktopEntryList parsedEntries,
                                           DesktopEntryList &catalog) const;

    [[nodiscard]] static std::string desktopFileIdFor(const Path &applicationsDir,
                                                      const Path &desktopFile);

    JobXdgPaths::UPtr m_paths;

    EntryList m_entries;
    DesktopEntryList m_desktopEntries;
};

} // namespace job::io