#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "pty_env_xdg.h"

namespace job::io {

class JobXdgPaths
{
public:
    using Ptr  = std::shared_ptr<JobXdgPaths>;
    using WPtr = std::weak_ptr<JobXdgPaths>;
    using UPtr = std::unique_ptr<JobXdgPaths>;

    using Path     = std::filesystem::path;
    using PathList = std::vector<Path>;

    JobXdgPaths() = default;
    ~JobXdgPaths() = default;

    JobXdgPaths(const JobXdgPaths &) = default;
    JobXdgPaths &operator=(const JobXdgPaths &) = default;
    JobXdgPaths(JobXdgPaths &&) noexcept = default;
    JobXdgPaths &operator=(JobXdgPaths &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobXdgPaths, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobXdgPaths>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobXdgPaths, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobXdgPaths>(std::forward<Args>(args)...);
    }

    [[nodiscard]] static Path dataHome();
    [[nodiscard]] static Path configHome();
    [[nodiscard]] static Path stateHome();
    [[nodiscard]] static Path cacheHome();
    [[nodiscard]] static std::optional<Path> runtimeDir();
    [[nodiscard]] static Path executableHome();

    [[nodiscard]] static PathList dataDirs();
    [[nodiscard]] static PathList configDirs();

    [[nodiscard]] static PathList dataSearchPaths();
    [[nodiscard]] static PathList configSearchPaths();
    [[nodiscard]] static PathList searchPaths(PtyEnvXdg::SearchGroup group);

    [[nodiscard]] static std::optional<Path> locate(PtyEnvXdg::SearchGroup group,
                                                    const Path &relativePath);

    [[nodiscard]] static PathList locateAll(PtyEnvXdg::SearchGroup group,
                                            const Path &relativePath);

    [[nodiscard]] static PathList applicationDirs();
    [[nodiscard]] static PathList desktopDirectoryDirs();
    [[nodiscard]] static PathList iconDirs();
    [[nodiscard]] static PathList mimeDirs();

    [[nodiscard]] static std::optional<Path> path(PtyEnvXdg::Env env);
    [[nodiscard]] static PathList paths(PtyEnvXdg::Env env);

    [[nodiscard]] static bool isAbsolute(const Path &path) noexcept;
    [[nodiscard]] static bool validate(PtyEnvXdg::Env env, const Path &path);
    [[nodiscard]] static bool validateRuntimeDir(const Path &path);

    [[nodiscard]] static Path dataPath(const Path &relativePath);
    [[nodiscard]] static Path configPath(const Path &relativePath);
    [[nodiscard]] static Path statePath(const Path &relativePath);
    [[nodiscard]] static Path cachePath(const Path &relativePath);

    [[nodiscard]] static bool ensureWritableDirectory(const Path &path);

private:
    [[nodiscard]] static std::optional<Path> resolveSingle(PtyEnvXdg::Env env);
    [[nodiscard]] static PathList resolveList(PtyEnvXdg::Env env);

    [[nodiscard]] static Path homeRelative(std::string_view relativePath);

    [[nodiscard]] static PathList makeSearchPaths(const Path &home,
                                                  const PathList &dirs);

    [[nodiscard]] static bool validRelativeResourcePath(const Path &path) noexcept;
};

} // namespace job::io