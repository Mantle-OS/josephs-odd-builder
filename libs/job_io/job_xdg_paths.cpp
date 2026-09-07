#include "job_xdg_paths.h"

#include <cstdlib>
#include <string>
#include <string_view>

#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>

#include <linux/magic.h>

#include "job_dir.h"
#include "job_syslink.h"

namespace job::io {

static bool isLocalFilesystem(const JobXdgPaths::Path &path) noexcept
{
    struct statfs info {};

    if (::statfs(path.c_str(), &info) == -1)
        return false;

    switch (static_cast<unsigned long>(info.f_type)) {
#ifdef NFS_SUPER_MAGIC
    case NFS_SUPER_MAGIC:
        return false;
#endif

#ifdef SMB_SUPER_MAGIC
    case SMB_SUPER_MAGIC:
        return false;
#endif

#ifdef CIFS_SUPER_MAGIC
    case CIFS_SUPER_MAGIC:
        return false;
#endif

#ifdef CODA_SUPER_MAGIC
    case CODA_SUPER_MAGIC:
        return false;
#endif

#ifdef AFS_SUPER_MAGIC
    case AFS_SUPER_MAGIC:
        return false;
#endif

    default:
        return true;
    }
}

static JobXdgPaths::PathList splitPathList(std::string_view value)
{
    JobXdgPaths::PathList paths;

    while (!value.empty()) {
        const std::size_t separator = value.find(':');
        const std::string_view part = separator == std::string_view::npos
                                          ? value
                                          : value.substr(0, separator);

        if (!part.empty()) {
            JobXdgPaths::Path path{part};

            if (path.is_absolute())
                paths.push_back(std::move(path));
        }

        if (separator == std::string_view::npos)
            break;

        value.remove_prefix(separator + 1);
    }

    return paths;
}

JobXdgPaths::Path JobXdgPaths::dataHome()
{
    return path(PtyEnvXdg::Env::DataHome).value_or(Path{});
}

JobXdgPaths::Path JobXdgPaths::configHome()
{
    return path(PtyEnvXdg::Env::ConfigHome).value_or(Path{});
}

JobXdgPaths::Path JobXdgPaths::stateHome()
{
    return path(PtyEnvXdg::Env::StateHome).value_or(Path{});
}

JobXdgPaths::Path JobXdgPaths::cacheHome()
{
    return path(PtyEnvXdg::Env::CacheHome).value_or(Path{});
}

std::optional<JobXdgPaths::Path> JobXdgPaths::runtimeDir()
{
    const auto result = path(PtyEnvXdg::Env::RuntimeDir);

    if (!result || !validateRuntimeDir(*result))
        return std::nullopt;

    return result;
}

JobXdgPaths::Path JobXdgPaths::executableHome()
{
    return homeRelative(".local/bin");
}

JobXdgPaths::PathList JobXdgPaths::dataDirs()
{
    return paths(PtyEnvXdg::Env::DataDirs);
}

JobXdgPaths::PathList JobXdgPaths::configDirs()
{
    return paths(PtyEnvXdg::Env::ConfigDirs);
}

JobXdgPaths::PathList JobXdgPaths::dataSearchPaths()
{
    return makeSearchPaths(dataHome(), dataDirs());
}

JobXdgPaths::PathList JobXdgPaths::configSearchPaths()
{
    return makeSearchPaths(configHome(), configDirs());
}

JobXdgPaths::PathList JobXdgPaths::searchPaths(PtyEnvXdg::SearchGroup group)
{
    switch (group) {
    case PtyEnvXdg::SearchGroup::Data:
        return dataSearchPaths();

    case PtyEnvXdg::SearchGroup::Config:
        return configSearchPaths();

    case PtyEnvXdg::SearchGroup::None:
    default:
        return {};
    }
}

std::optional<JobXdgPaths::Path> JobXdgPaths::locate(PtyEnvXdg::SearchGroup group,
                                                     const Path &relativePath)
{
    if (!validRelativeResourcePath(relativePath))
        return std::nullopt;

    for (const auto &base : searchPaths(group)) {
        if (base.empty())
            continue;

        const Path candidate = base / relativePath;

        if (::access(candidate.c_str(), R_OK) == 0)
            return candidate;
    }

    return std::nullopt;
}

JobXdgPaths::PathList JobXdgPaths::locateAll(PtyEnvXdg::SearchGroup group,
                                             const Path &relativePath)
{
    PathList matches;

    if (!validRelativeResourcePath(relativePath))
        return matches;

    for (const auto &base : searchPaths(group)) {
        if (base.empty())
            continue;

        const Path candidate = base / relativePath;

        if (::access(candidate.c_str(), R_OK) == 0)
            matches.push_back(candidate);
    }

    return matches;
}

JobXdgPaths::PathList JobXdgPaths::applicationDirs()
{
    PathList result;

    for (const auto &base : dataSearchPaths()) {
        if (!base.empty())
            result.push_back(base / "applications");
    }

    return result;
}

JobXdgPaths::PathList JobXdgPaths::desktopDirectoryDirs()
{
    PathList result;

    for (const auto &base : dataSearchPaths()) {
        if (!base.empty())
            result.push_back(base / "desktop-directories");
    }

    return result;
}

JobXdgPaths::PathList JobXdgPaths::iconDirs()
{
    PathList result;

    for (const auto &base : dataSearchPaths()) {
        if (!base.empty())
            result.push_back(base / "icons");
    }

    return result;
}

JobXdgPaths::PathList JobXdgPaths::mimeDirs()
{
    PathList result;

    for (const auto &base : dataSearchPaths()) {
        if (!base.empty())
            result.push_back(base / "mime");
    }

    return result;
}

std::optional<JobXdgPaths::Path> JobXdgPaths::path(PtyEnvXdg::Env env)
{
    if (PtyEnvXdg::valueType(env) != PtyEnvXdg::ValueType::Path)
        return std::nullopt;

    return resolveSingle(env);
}

JobXdgPaths::PathList JobXdgPaths::paths(PtyEnvXdg::Env env)
{
    if (PtyEnvXdg::valueType(env) != PtyEnvXdg::ValueType::PathList)
        return {};

    return resolveList(env);
}

bool JobXdgPaths::isAbsolute(const Path &path) noexcept
{
    return !path.empty() && path.is_absolute();
}

bool JobXdgPaths::validate(PtyEnvXdg::Env env, const Path &path)
{
    const PtyEnvXdg::Constraint constraints = PtyEnvXdg::constraints(env);

    if (hasConstraint(constraints, PtyEnvXdg::Constraint::AbsolutePath) && !isAbsolute(path))
        return false;

    if (!hasConstraint(constraints, PtyEnvXdg::Constraint::UserOwned) &&
        !hasConstraint(constraints, PtyEnvXdg::Constraint::PrivateMode) &&
        !hasConstraint(constraints, PtyEnvXdg::Constraint::LocalFilesystem)) {
        return true;
    }

    JobDir dir(path);

    if (!dir.exists() || !dir.isDirectory())
        return false;

    if (hasConstraint(constraints, PtyEnvXdg::Constraint::PrivateMode) &&
        !dir.hasPermissions(IOPermissions::PrivateDirectory)) {
        return false;
    }

    if (hasConstraint(constraints, PtyEnvXdg::Constraint::UserOwned)) {
        struct stat info {};

        if (::stat(path.c_str(), &info) == -1)
            return false;

        if (info.st_uid != ::geteuid())
            return false;
    }

    if (hasConstraint(constraints, PtyEnvXdg::Constraint::LocalFilesystem) &&
        !isLocalFilesystem(path)) {
        return false;
    }

    return true;
}

bool JobXdgPaths::validateRuntimeDir(const Path &path)
{
    if (!isAbsolute(path))
        return false;

    /*
     * XDG_RUNTIME_DIR itself must be the directory. Do not accept a symlink
     * which merely resolves to an otherwise valid runtime directory.
     */
    if (JobSysLink::isSysLink(path))
        return false;

    JobDir dir(path);

    if (!dir.exists() || !dir.isDirectory())
        return false;

    if (!dir.hasPermissions(IOPermissions::PrivateDirectory))
        return false;

    struct stat info {};

    if (::stat(path.c_str(), &info) == -1)
        return false;

    if (info.st_uid != ::geteuid())
        return false;

    if (!isLocalFilesystem(path))
        return false;

    return true;
}

JobXdgPaths::Path JobXdgPaths::dataPath(const Path &relativePath)
{
    if (!validRelativeResourcePath(relativePath))
        return {};

    const Path base = dataHome();

    if (base.empty())
        return {};

    return base / relativePath;
}

JobXdgPaths::Path JobXdgPaths::configPath(const Path &relativePath)
{
    if (!validRelativeResourcePath(relativePath))
        return {};

    const Path base = configHome();

    if (base.empty())
        return {};

    return base / relativePath;
}

JobXdgPaths::Path JobXdgPaths::statePath(const Path &relativePath)
{
    if (!validRelativeResourcePath(relativePath))
        return {};

    const Path base = stateHome();

    if (base.empty())
        return {};

    return base / relativePath;
}

JobXdgPaths::Path JobXdgPaths::cachePath(const Path &relativePath)
{
    if (!validRelativeResourcePath(relativePath))
        return {};

    const Path base = cacheHome();

    if (base.empty())
        return {};

    return base / relativePath;
}

bool JobXdgPaths::ensureWritableDirectory(const Path &path)
{
    if (!isAbsolute(path))
        return false;

    JobDir dir(path);

    /*
     * Existing XDG directories retain their existing permissions.
     */
    if (dir.exists())
        return dir.isDirectory();

    /*
     * JobDir::createParents(perms) applies the requested permissions only to
     * directories it creates and leaves existing parents untouched.
     */
    return dir.createParents(IOPermissions::PrivateDirectory);
}

std::optional<JobXdgPaths::Path> JobXdgPaths::resolveSingle(PtyEnvXdg::Env env)
{
    const std::string envName{PtyEnvXdg::name(env)};
    const char *value = ::getenv(envName.c_str());

    if (value && *value != '\0') {
        Path result{value};

        if (isAbsolute(result))
            return result;
    }

    switch (PtyEnvXdg::defaultType(env)) {
    case PtyEnvXdg::DefaultType::HomeRelative:
        return homeRelative(PtyEnvXdg::defaultValue(env));

    case PtyEnvXdg::DefaultType::Absolute: {
        Path result{PtyEnvXdg::defaultValue(env)};

        if (isAbsolute(result))
            return result;

        return std::nullopt;
    }

    case PtyEnvXdg::DefaultType::None:
    default:
        return std::nullopt;
    }
}

JobXdgPaths::PathList JobXdgPaths::resolveList(PtyEnvXdg::Env env)
{
    const std::string envName{PtyEnvXdg::name(env)};
    const char *value = ::getenv(envName.c_str());

    if (value && *value != '\0')
        return splitPathList(value);

    return splitPathList(PtyEnvXdg::defaultValue(env));
}

JobXdgPaths::Path JobXdgPaths::homeRelative(std::string_view relativePath)
{
    const char *home = ::getenv("HOME");

    if (!home || *home == '\0')
        return {};

    Path homePath{home};

    if (!homePath.is_absolute())
        return {};

    if (relativePath.empty())
        return homePath;

    return homePath / Path{relativePath};
}

JobXdgPaths::PathList JobXdgPaths::makeSearchPaths(const Path &home,
                                                   const PathList &dirs)
{
    PathList result;
    result.reserve(dirs.size() + (home.empty() ? 0 : 1));

    if (!home.empty())
        result.push_back(home);

    for (const auto &dir : dirs) {
        if (!dir.empty())
            result.push_back(dir);
    }

    return result;
}

bool JobXdgPaths::validRelativeResourcePath(const Path &path) noexcept
{
    if (path.empty() || path.is_absolute())
        return false;

    for (const auto &component : path) {
        if (component == "..")
            return false;
    }

    return true;
}

} // namespace job::io