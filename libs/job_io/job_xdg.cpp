#include "job_xdg.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <system_error>
#include <unordered_set>

#include <unistd.h>

#include <job_parallel_for.h>

#include "job_dir.h"
#include "job_file.h"

namespace job::io {

JobXdg &JobXdg::instance()
{
    static JobXdg xdg;
    return xdg;
}

JobXdg::JobXdg() :
    m_paths(JobXdgPaths::createUniq())
{
    (void)reload();
}

const JobXdgPaths &JobXdg::paths() const noexcept
{
    return *m_paths;
}

const JobXdg::EntryList &JobXdg::entries() const noexcept
{
    return m_entries;
}

const JobXdg::DesktopEntryList &JobXdg::desktopEntries() const noexcept
{
    return m_desktopEntries;
}

const JobXdgEntry *JobXdg::find(std::string_view desktopFileId) const noexcept
{
    for (const auto &entry : m_entries) {
        if (entry.desktopFileId() == desktopFileId)
            return &entry;
    }

    return nullptr;
}

bool JobXdg::contains(std::string_view desktopFileId) const noexcept
{
    return find(desktopFileId) != nullptr;
}

std::size_t JobXdg::size() const noexcept
{
    return m_entries.size();
}

bool JobXdg::empty() const noexcept
{
    return m_entries.empty();
}

std::optional<XdgExec> JobXdg::exec(std::string_view desktopFileId) const
{
    const JobXdgEntry *entry = find(desktopFileId);

    if (!entry)
        return std::nullopt;

    return exec(*entry);
}

std::optional<XdgExec> JobXdg::exec(const JobXdgEntry &entry) const
{
    if (entry.type() != JobXdgEntryType::Application || entry.exec().empty())
        return std::nullopt;

    if (!entry.tryExec().empty()) {
        const Path tryExec{entry.tryExec()};
        bool available = false;

        if (tryExec.is_absolute()) {
            available = ::access(tryExec.c_str(), X_OK) == 0;
        } else {
            const char *pathEnv = ::getenv("PATH");

            if (pathEnv && *pathEnv != '\0') {
                std::string_view searchPath{pathEnv};

                while (true) {
                    const std::size_t separator = searchPath.find(':');
                    const std::string_view component = separator == std::string_view::npos
                                                           ? searchPath
                                                           : searchPath.subview(0, separator);

                    const Path base = component.empty() ? Path{"."} : Path{component};
                    const Path candidate = base / tryExec;

                    if (::access(candidate.c_str(), X_OK) == 0) {
                        available = true;
                        break;
                    }

                    if (separator == std::string_view::npos)
                        break;

                    searchPath.remove_prefix(separator + 1);
                }
            }
        }

        if (!available)
            return std::nullopt;
    }

    std::vector<std::string> tokens;

    if (!splitExec(entry.exec(), tokens) || tokens.empty())
        return std::nullopt;

    std::vector<std::string> expanded;
    expanded.reserve(tokens.size());

    for (const auto &token : tokens) {
        if (!expandExecField(token, entry, expanded))
            return std::nullopt;
    }

    if (expanded.empty() || expanded.front().empty() ||
        expanded.front().find('=') != std::string::npos) {
        return std::nullopt;
    }

    XdgExec result;
    result.program = std::move(expanded.front());
    result.arguments.reserve(expanded.size() - 1);

    for (std::size_t i = 1; i < expanded.size(); ++i)
        result.arguments.push_back(std::move(expanded[i]));

    if (!entry.workingDirectory().empty())
        result.workingDirectory = Path{entry.workingDirectory()};

    result.terminal = entry.terminal();
    return result;
}

bool JobXdg::reload()
{
    auto newPaths = JobXdgPaths::createUniq();
    JobXdgPaths::UPtr oldPaths = std::move(m_paths);
    m_paths = std::move(newPaths);

    const PathList files = discoverDesktopFiles();

    EntryList parsed;

    if (!loadSerial(files, parsed)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    EntryList catalog;

    if (!buildCatalog(std::move(parsed), catalog)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    DesktopEntryList parsedDesktopEntries;
    if (!loadDesktopEntries(files, parsedDesktopEntries)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    DesktopEntryList desktopCatalog;
    if (!buildDesktopCatalog(std::move(parsedDesktopEntries), desktopCatalog)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    m_entries = std::move(catalog);
    m_desktopEntries = std::move(desktopCatalog);

    return true;
}

bool JobXdg::reloadParallel(job::threads::ThreadPool &pool)
{
    auto newPaths = JobXdgPaths::createUniq();
    JobXdgPaths::UPtr oldPaths = std::move(m_paths);
    m_paths = std::move(newPaths);

    const PathList files = discoverDesktopFiles();

    std::string buffer;
    SourceList sources;

    if (!loadSources(files, buffer, sources)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    EntryList parsed;

    if (!parseSourcesParallel(pool, buffer, sources, parsed)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    EntryList catalog;

    if (!buildCatalog(std::move(parsed), catalog)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    DesktopEntryList parsedDesktopEntries;
    if (!loadDesktopEntries(files, parsedDesktopEntries)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    DesktopEntryList desktopCatalog;
    if (!buildDesktopCatalog(std::move(parsedDesktopEntries), desktopCatalog)) {
        m_paths = std::move(oldPaths);
        return false;
    }

    m_entries = std::move(catalog);
    m_desktopEntries = std::move(desktopCatalog);
    return true;
}

JobXdg::PathList JobXdg::discoverDesktopFiles() const
{
    PathList result;

    for (const auto &applicationsDir : m_paths->applicationDirs()) {
        JobDir dir(applicationsDir);

        if (!dir.exists() || !dir.isDirectory())
            continue;

        PathList current;
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(
            applicationsDir,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        const std::filesystem::recursive_directory_iterator end;

        while (!ec && it != end) {
            const auto &entry = *it;
            std::error_code entryError;
            const bool regular = entry.is_regular_file(entryError);

            if (!entryError && regular && entry.path().extension() == ".desktop")
                current.push_back(entry.path());

            it.increment(ec);
        }

        std::sort(current.begin(), current.end());
        result.insert(result.end(), std::make_move_iterator(current.begin()),
                      std::make_move_iterator(current.end()));
    }

    return result;
}

bool JobXdg::loadSerial(const PathList &files, EntryList &entries) const
{
    entries.clear();
    entries.reserve(files.size());

    for (const auto &path : files) {
        JobXdgEntry entry;

        if (loadEntry(path, entry))
            entries.push_back(std::move(entry));
    }

    return true;
}

bool JobXdg::loadSources(const PathList &files, std::string &buffer, SourceList &sources) const
{
    buffer.clear();
    sources.clear();
    sources.reserve(files.size());

    for (const auto &path : files) {
        JobFile file(path, JobFile::Access::ReadOnly);

        if (!file.openDevice())
            continue;

        std::string source;

        if (file.readAll(source) < 0)
            continue;

        const std::size_t offset = buffer.size();
        const std::size_t size = source.size();
        buffer.append(source);

        sources.push_back({.path = path, .offset = offset, .size = size});
    }

    return true;
}

bool JobXdg::parseSourcesParallel(job::threads::ThreadPool &pool,
                                  std::string_view buffer,
                                  const SourceList &sources,
                                  EntryList &entries) const
{
    entries.clear();

    if (sources.empty())
        return true;

    std::vector<std::optional<JobXdgEntry>> parsed(sources.size());

    job::threads::parallel_for(pool, 0, sources.size(), [&](std::size_t i) {
        const Source &source = sources[i];

        if (source.offset > buffer.size() || source.size > buffer.size() - source.offset)
            return;

        JobXdgEntry entry;
        const std::string_view sourceView = buffer.subview(source.offset, source.size);

        if (parseEntry(source.path, sourceView, entry))
            parsed[i] = std::move(entry);
    });

    entries.reserve(sources.size());

    for (auto &entry : parsed) {
        if (entry)
            entries.push_back(std::move(*entry));
    }

    return true;
}

bool JobXdg::loadEntry(const Path &path, JobXdgEntry &entry) const
{
    JobFile file(path, JobFile::Access::ReadOnly);

    if (!file.openDevice())
        return false;

    std::string source;

    if (file.readAll(source) < 0)
        return false;

    return parseEntry(path, source, entry);
}


bool JobXdg::loadDesktopEntries(const PathList &files,
                                DesktopEntryList &entries) const
{
    entries.clear();
    entries.reserve(files.size());

    for (const auto &path : files) {
        JobXdgDesktopEntry entry;

        if (loadDesktopEntry(path, entry))
            entries.push_back(std::move(entry));
    }

    return true;
}

bool JobXdg::loadDesktopEntry(const Path &path,
                              JobXdgDesktopEntry &entry) const
{
    JobFile file(path, JobFile::Access::ReadOnly);

    if (!file.openDevice())
        return false;

    std::string source;

    if (file.readAll(source) < 0)
        return false;

    entry = JobXdgDesktopEntry{};

    if (!entry.fromIni(source))
        return false;

    entry.path = path;

    for (const auto &applicationsDir : m_paths->applicationDirs()) {
        std::string id = desktopFileIdFor(applicationsDir, path);

        if (!id.empty()) {
            entry.desktopFileId = std::move(id);
            break;
        }
    }

    return !entry.desktopFileId.empty();
}





bool JobXdg::parseEntry(const Path &path, std::string_view source, JobXdgEntry &entry) const
{
    entry = JobXdgEntry{};
    entry.m_path = path;

    for (const auto &applicationsDir : m_paths->applicationDirs()) {
        const std::string id = desktopFileIdFor(applicationsDir, path);

        if (!id.empty()) {
            entry.m_desktopFileId = id;
            break;
        }
    }

    if (entry.m_desktopFileId.empty())
        return false;

    std::string currentGroup;
    std::unordered_set<std::string> groups;
    std::unordered_set<std::string> keys;

    bool hasDesktopEntry = false;
    bool hasType = false;
    bool hasName = false;

    auto trim = [](std::string_view value) noexcept {
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
            value.remove_prefix(1);
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
            value.remove_suffix(1);
        return value;
    };

    auto splitLocalizedKey = [](std::string_view key,
                                std::string_view &base,
                                std::string_view &locale) noexcept {
        base = key;
        locale = {};
        const std::size_t open = key.find('[');

        if (open == std::string_view::npos)
            return true;
        if (open == 0 || key.back() != ']')
            return false;

        base = key.subview(0, open);
        locale = key.subview(open + 1, key.size() - open - 2);
        return !locale.empty();
    };

    auto assignLocalized = [&](JobXdgLocalizedString &destination,
                               std::string_view key,
                               std::string_view expected,
                               std::string_view value) {
        std::string_view base;
        std::string_view locale;

        if (!splitLocalizedKey(key, base, locale) || base != expected)
            return false;

        std::string decoded = unescape(value);

        if (locale.empty())
            destination.value = std::move(decoded);
        else
            destination.localized[std::string(locale)] = std::move(decoded);

        return true;
    };

    auto findAction = [&](std::string_view id) -> JobXdgAction * {
        for (auto &action : entry.m_desktopActions) {
            if (action.id == id)
                return &action;
        }
        return nullptr;
    };

    std::size_t position = 0;

    while (position <= source.size()) {
        const std::size_t newline = source.find('\n', position);
        const std::size_t length = newline == std::string_view::npos
                                       ? source.size() - position
                                       : newline - position;
        std::string_view line = source.subview(position, length);

        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        position = newline == std::string_view::npos ? source.size() + 1 : newline + 1;

        if (line.empty() || line.front() == '#')
            continue;

        if (line.front() == '[') {
            if (line.size() < 3 || line.back() != ']')
                return false;

            const std::string_view group = line.subview(1, line.size() - 2);

            if (group.empty() || group.find('[') != std::string_view::npos ||
                group.find(']') != std::string_view::npos) {
                return false;
            }

            std::string groupName{group};

            if (!groups.insert(groupName).second)
                return false;

            currentGroup = std::move(groupName);
            keys.clear();

            if (currentGroup == "Desktop Entry") {
                hasDesktopEntry = true;
            } else if (currentGroup.starts_with("Desktop Action ")) {
                const std::string_view id = std::string_view{currentGroup}.subview(15);

                if (!id.empty()) {
                    JobXdgAction action;
                    action.id.assign(id);
                    entry.m_desktopActions.push_back(std::move(action));
                }
            } else {
                JobXdgRawGroup groupEntry;
                groupEntry.name = currentGroup;
                entry.m_extensionGroups.push_back(std::move(groupEntry));
            }

            continue;
        }

        if (currentGroup.empty())
            return false;

        const std::size_t equals = line.find('=');

        if (equals == std::string_view::npos)
            return false;

        const std::string_view key = trim(line.subview(0, equals));
        const std::string_view value = trim(line.subview(equals + 1));

        if (key.empty() || !keys.insert(std::string(key)).second)
            return false;

        if (currentGroup == "Desktop Entry") {
            std::string_view baseKey;
            std::string_view locale;

            if (!splitLocalizedKey(key, baseKey, locale))
                return false;

            if (baseKey == "Name") {
                if (!assignLocalized(entry.m_name, key, "Name", value))
                    return false;
                if (locale.empty())
                    hasName = true;
                continue;
            }
            if (baseKey == "GenericName") {
                if (!assignLocalized(entry.m_genericName, key, "GenericName", value))
                    return false;
                continue;
            }
            if (baseKey == "Comment") {
                if (!assignLocalized(entry.m_comment, key, "Comment", value))
                    return false;
                continue;
            }
            if (baseKey == "Icon") {
                if (!assignLocalized(entry.m_icon, key, "Icon", value))
                    return false;
                continue;
            }
            if (baseKey == "Keywords") {
                if (!assignLocalized(entry.m_keywords, key, "Keywords", value))
                    return false;
                continue;
            }

            if (!locale.empty()) {
                JobXdgRawEntry raw;
                raw.key.assign(key);
                raw.value.assign(value);
                entry.m_extensionEntries.push_back(std::move(raw));
                // entry.m_extensionEntries.push_back({.key = std::string(key), .value = std::string(value)});
                continue;
            }

            if (key == "Type") {
                entry.m_type = parseType(value);
                hasType = true;
            } else if (key == "Version") {
                entry.m_version = unescape(value);
            } else if (key == "NoDisplay") {
                if (!parseBoolean(value, entry.m_noDisplay))
                    return false;
            } else if (key == "Hidden") {
                if (!parseBoolean(value, entry.m_hidden))
                    return false;
            } else if (key == "OnlyShowIn") {
                entry.m_onlyShowIn = parseList(value);
            } else if (key == "NotShowIn") {
                entry.m_notShowIn = parseList(value);
            } else if (key == "DBusActivatable") {
                if (!parseBoolean(value, entry.m_dbusActivatable))
                    return false;
            } else if (key == "TryExec") {
                entry.m_tryExec = unescape(value);
            } else if (key == "Exec") {
                entry.m_exec = unescape(value);
            } else if (key == "Path") {
                entry.m_workingDirectory = unescape(value);
            } else if (key == "Terminal") {
                if (!parseBoolean(value, entry.m_terminal))
                    return false;
            } else if (key == "Actions") {
                entry.m_actions = parseList(value);
            } else if (key == "MimeType") {
                entry.m_mimeTypes = parseList(value);
            } else if (key == "Categories") {
                entry.m_categories = parseList(value);
            } else if (key == "Implements") {
                entry.m_implements = parseList(value);
            } else if (key == "StartupNotify") {
                bool startupNotify = false;
                if (!parseBoolean(value, startupNotify))
                    return false;
                entry.m_startupNotify = startupNotify;
            } else if (key == "StartupWMClass") {
                entry.m_startupWMClass = unescape(value);
            } else if (key == "URL") {
                entry.m_url = unescape(value);
            } else if (key == "PrefersNonDefaultGPU") {
                if (!parseBoolean(value, entry.m_prefersNonDefaultGPU))
                    return false;
            } else if (key == "SingleMainWindow") {
                if (!parseBoolean(value, entry.m_singleMainWindow))
                    return false;
            } else {
                JobXdgRawEntry raw;
                raw.key.assign(key);
                raw.value.assign(value);
                entry.m_extensionEntries.push_back(std::move(raw));
                // entry.m_extensionEntries.push_back({.key = std::string(key), .value = std::string(value)});
            }

            continue;
        }

        if (currentGroup.starts_with("Desktop Action ")) {
            const std::string_view id = std::string_view{currentGroup}.subview(15);
            JobXdgAction *action = findAction(id);

            if (!action)
                continue;

            std::string_view baseKey;
            std::string_view locale;

            if (!splitLocalizedKey(key, baseKey, locale))
                return false;

            if (baseKey == "Name") {
                if (!assignLocalized(action->name, key, "Name", value))
                    return false;
            } else if (baseKey == "Icon") {
                if (!assignLocalized(action->icon, key, "Icon", value))
                    return false;
            } else if (key == "Exec" && locale.empty()) {
                action->exec = unescape(value);
            } else {
                JobXdgRawEntry raw;
                raw.key.assign(key);
                raw.value.assign(value);
                action->extensionEntries.push_back(std::move(raw));
            }

            continue;
        }

        for (auto &group : entry.m_extensionGroups) {
            if (group.name == currentGroup) {
                JobXdgRawEntry raw;
                raw.key.assign(key);
                raw.value.assign(value);
                group.entries.push_back(std::move(raw));
                break;
            }
        }
    }

    if (!hasDesktopEntry || !hasType || !hasName)
        return false;
    if (entry.m_type == JobXdgEntryType::Unknown)
        return false;
    if (entry.m_type == JobXdgEntryType::Application && entry.m_exec.empty() && !entry.m_dbusActivatable)
        return false;
    if (entry.m_type == JobXdgEntryType::Link && entry.m_url.empty())
        return false;

    std::erase_if(entry.m_desktopActions, [&](const JobXdgAction &action) {
        const bool listed = std::find(entry.m_actions.begin(), entry.m_actions.end(), action.id) != entry.m_actions.end();
        return !listed || action.name.value.empty();
    });

    return true;
}

bool JobXdg::buildCatalog(EntryList parsedEntries, EntryList &catalog) const
{
    catalog.clear();
    catalog.reserve(parsedEntries.size());

    std::unordered_set<std::string> seenDesktopIds;
    seenDesktopIds.reserve(parsedEntries.size());

    for (auto &entry : parsedEntries) {
        const std::string &id = entry.desktopFileId();

        if (id.empty() || !seenDesktopIds.insert(id).second)
            continue;

        if (entry.hidden())
            continue;

        catalog.push_back(std::move(entry));
    }

    return true;
}


bool JobXdg::buildDesktopCatalog(DesktopEntryList parsedEntries,
                                 DesktopEntryList &catalog) const
{
    catalog.clear();
    catalog.reserve(parsedEntries.size());

    std::unordered_set<std::string> seenDesktopIds;
    seenDesktopIds.reserve(parsedEntries.size());

    for (auto &entry : parsedEntries) {
        const std::string &id = entry.desktopFileId;

        if (id.empty() || !seenDesktopIds.insert(id).second)
            continue;

        if (entry.Hidden)
            continue;

        catalog.push_back(std::move(entry));
    }

    return true;
}

bool JobXdg::parseBoolean(std::string_view value, bool &result) noexcept
{
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

JobXdgEntryType JobXdg::parseType(std::string_view value) noexcept
{
    if (value == "Application")
        return JobXdgEntryType::Application;
    if (value == "Link")
        return JobXdgEntryType::Link;
    if (value == "Directory")
        return JobXdgEntryType::Directory;
    return JobXdgEntryType::Unknown;
}

std::string JobXdg::unescape(std::string_view value)
{
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            result.push_back(value[i]);
            continue;
        }

        const char escaped = value[++i];

        switch (escaped) {
        case 's': result.push_back(' '); break;
        case 'n': result.push_back('\n'); break;
        case 't': result.push_back('\t'); break;
        case 'r': result.push_back('\r'); break;
        case '\\': result.push_back('\\'); break;
        case ';': result.push_back(';'); break;
        default:
            result.push_back('\\');
            result.push_back(escaped);
            break;
        }
    }

    return result;
}

std::vector<std::string> JobXdg::parseList(std::string_view value)
{
    std::vector<std::string> result;
    std::string current;
    bool escaped = false;
    bool endedWithSeparator = false;
    current.reserve(value.size());

    for (const char c : value) {
        if (escaped) {
            switch (c) {
            case 's': current.push_back(' '); break;
            case 'n': current.push_back('\n'); break;
            case 't': current.push_back('\t'); break;
            case 'r': current.push_back('\r'); break;
            case '\\': current.push_back('\\'); break;
            case ';': current.push_back(';'); break;
            default:
                current.push_back('\\');
                current.push_back(c);
                break;
            }
            escaped = false;
            endedWithSeparator = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == ';') {
            result.push_back(std::move(current));
            current.clear();
            endedWithSeparator = true;
            continue;
        }

        current.push_back(c);
        endedWithSeparator = false;
    }

    if (escaped)
        current.push_back('\\');
    if (!endedWithSeparator)
        result.push_back(std::move(current));

    return result;
}

bool JobXdg::splitExec(std::string_view value, std::vector<std::string> &arguments)
{
    arguments.clear();

    std::string current;
    bool quoted = false;

    auto flush = [&]() {
        if (!current.empty()) {
            arguments.push_back(std::move(current));
            current.clear();
        }
    };

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];

        if (quoted) {
            if (c == '"') {
                quoted = false;
                continue;
            }
            if (c == '\\') {
                if (i + 1 >= value.size())
                    return false;
                const char escaped = value[++i];
                switch (escaped) {
                case '"':
                case '`':
                case '$':
                case '\\':
                    current.push_back(escaped);
                    break;
                default:
                    current.push_back('\\');
                    current.push_back(escaped);
                    break;
                }
                continue;
            }
            if (c == '%' && i + 1 < value.size() && value[i + 1] != '%')
                return false;
            current.push_back(c);
            continue;
        }

        if (c == '"') {
            quoted = true;
            continue;
        }
        if (c == ' ' || c == '\t') {
            flush();
            continue;
        }

        current.push_back(c);
    }

    if (quoted)
        return false;

    flush();
    return !arguments.empty();
}

bool JobXdg::expandExecField(std::string_view token,
                             const JobXdgEntry &entry,
                             std::vector<std::string> &arguments)
{
    if (token == "%i") {
        if (!entry.icon().value.empty()) {
            arguments.emplace_back("--icon");
            arguments.push_back(entry.icon().value);
        }
        return true;
    }

    if (token == "%F" || token == "%U")
        return true;

    std::string expanded;
    expanded.reserve(token.size());

    for (std::size_t i = 0; i < token.size(); ++i) {
        if (token[i] != '%') {
            expanded.push_back(token[i]);
            continue;
        }

        if (i + 1 >= token.size())
            return false;

        const char code = token[++i];

        switch (code) {
        case '%': expanded.push_back('%'); break;
        case 'c': expanded.append(entry.name().value); break;
        case 'k': expanded.append(entry.path().string()); break;
        case 'f':
        case 'u':
            break;
        case 'F':
        case 'U':
        case 'i':
            return false;
        case 'd':
        case 'D':
        case 'n':
        case 'N':
        case 'v':
        case 'm':
            break;
        default:
            return false;
        }
    }

    if (!expanded.empty())
        arguments.push_back(std::move(expanded));

    return true;
}

std::string JobXdg::desktopFileIdFor(const Path &applicationsDir, const Path &desktopFile)
{
    if (applicationsDir.empty() || desktopFile.empty())
        return {};

    std::error_code ec;
    Path relative = std::filesystem::relative(desktopFile, applicationsDir, ec);

    if (ec || relative.empty() || relative.is_absolute())
        return {};

    for (const auto &component : relative) {
        if (component == "..")
            return {};
    }

    std::string id = relative.generic_string();
    std::replace(id.begin(), id.end(), '/', '-');
    return id;
}

} // namespace job::io
