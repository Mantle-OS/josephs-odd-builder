#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "job_permissions.h"

namespace job::io {

class JobDir
{
public:
    using Ptr  = std::shared_ptr<JobDir>;
    using WPtr = std::weak_ptr<JobDir>;
    using UPtr = std::unique_ptr<JobDir>;

    using Path     = std::filesystem::path;
    using PathList = std::vector<Path>;

    JobDir() = default;
    explicit JobDir(Path path);
    ~JobDir() = default;

    JobDir(const JobDir &) = default;
    JobDir &operator=(const JobDir &) = default;
    JobDir(JobDir &&) noexcept = default;
    JobDir &operator=(JobDir &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobDir, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobDir>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobDir, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobDir>(std::forward<Args>(args)...);
    }

    // Path -------------------------------------------------------------------

    [[nodiscard]] const Path &path() const noexcept;
    void setPath(Path path);

    [[nodiscard]] bool empty() const noexcept;

    // State ------------------------------------------------------------------

    [[nodiscard]] bool exists() const noexcept;
    [[nodiscard]] bool isDirectory() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] bool isReadable() const noexcept;
    [[nodiscard]] bool isWritable() const noexcept;
    [[nodiscard]] bool isExecutable() const noexcept;

    [[nodiscard]] bool isAbsolute() const noexcept;
    [[nodiscard]] bool isRelative() const noexcept;

    // Creation ---------------------------------------------------------------

    [[nodiscard]] bool create();
    [[nodiscard]] bool create(IOPermissions permissions);

    [[nodiscard]] bool createParents();
    [[nodiscard]] bool createParents(IOPermissions permissions);

    // Removal ----------------------------------------------------------------

    [[nodiscard]] bool remove();
    [[nodiscard]] bool removeRecursive();

    // Contents ---------------------------------------------------------------

    [[nodiscard]] PathList entries() const;
    [[nodiscard]] PathList files() const;
    [[nodiscard]] PathList directories() const;

    // Permissions ------------------------------------------------------------

    [[nodiscard]] IOPermissions permissions() const noexcept;
    [[nodiscard]] bool setPermissions(IOPermissions permissions) noexcept;
    [[nodiscard]] bool hasPermissions(IOPermissions permissions) const noexcept;

    // Static convenience -----------------------------------------------------

    [[nodiscard]] static bool exists(const Path &path) noexcept;
    [[nodiscard]] static bool isDirectory(const Path &path) noexcept;
    [[nodiscard]] static bool isEmpty(const Path &path) noexcept;

    [[nodiscard]] static bool create(const Path &path);
    [[nodiscard]] static bool create(const Path &path, IOPermissions permissions);

    [[nodiscard]] static bool createParents(const Path &path);
    [[nodiscard]] static bool createParents(const Path &path, IOPermissions permissions);

    [[nodiscard]] static bool remove(const Path &path);
    [[nodiscard]] static bool removeRecursive(const Path &path);

private:
    Path m_path;
};

} // namespace job::io