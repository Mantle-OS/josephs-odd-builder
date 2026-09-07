#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <utility>

#include "job_dir.h"

namespace job::io {

class JobTmpDir final
{
public:
    using Ptr  = std::shared_ptr<JobTmpDir>;
    using WPtr = std::weak_ptr<JobTmpDir>;
    using UPtr = std::unique_ptr<JobTmpDir>;

    using Path = std::filesystem::path;

    explicit JobTmpDir(Path path) :
        JobTmpDir(std::move(path), IOPermissions::PrivateDirectory)
    {
    }

    JobTmpDir(Path path, IOPermissions permissions) :
        m_path(std::move(path)),
        m_dir(m_path)
    {
        create(permissions);
    }

    ~JobTmpDir()
    {
        cleanup();
    }

    JobTmpDir(const JobTmpDir &) = delete;
    JobTmpDir &operator=(const JobTmpDir &) = delete;

    JobTmpDir(JobTmpDir &&other) noexcept :
        m_path(std::move(other.m_path)),
        m_dir(std::move(other.m_dir)),
        m_removeOnDestroy(other.m_removeOnDestroy)
    {
        other.m_path.clear();
        other.m_dir.setPath({});
        other.m_removeOnDestroy = false;
    }

    JobTmpDir &operator=(JobTmpDir &&other) noexcept
    {
        if (this != &other) {
            cleanup();

            m_path = std::move(other.m_path);
            m_dir = std::move(other.m_dir);
            m_removeOnDestroy = other.m_removeOnDestroy;

            other.m_path.clear();
            other.m_dir.setPath({});
            other.m_removeOnDestroy = false;
        }

        return *this;
    }

    [[nodiscard]] static Ptr createShared(Path path)
    {
        return std::make_shared<JobTmpDir>(std::move(path));
    }

    [[nodiscard]] static Ptr createShared(Path path, IOPermissions permissions)
    {
        return std::make_shared<JobTmpDir>(std::move(path), permissions);
    }

    [[nodiscard]] static UPtr createUniq(Path path)
    {
        return std::make_unique<JobTmpDir>(std::move(path));
    }

    [[nodiscard]] static UPtr createUniq(Path path, IOPermissions permissions)
    {
        return std::make_unique<JobTmpDir>(std::move(path), permissions);
    }

    [[nodiscard]] const Path &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] std::string pathString() const
    {
        return m_path.string();
    }

    [[nodiscard]] JobDir &dir() noexcept
    {
        return m_dir;
    }

    [[nodiscard]] const JobDir &dir() const noexcept
    {
        return m_dir;
    }

    [[nodiscard]] bool exists() const noexcept
    {
        return m_dir.exists();
    }

    [[nodiscard]] bool removeOnDestroy() const noexcept
    {
        return m_removeOnDestroy;
    }

    void setRemoveOnDestroy(bool enabled) noexcept
    {
        m_removeOnDestroy = enabled;
    }

private:
    void create(IOPermissions permissions)
    {
        if (m_path.empty())
            throw std::runtime_error("Temporary directory path is empty");

        if (!m_dir.createParents(permissions))
            throw std::runtime_error("Failed to create temporary directory: " + m_path.string());
    }

    void cleanup() noexcept
    {
        if (!m_removeOnDestroy || m_path.empty())
            return;

        (void)m_dir.removeRecursive();

        m_path.clear();
        m_dir.setPath({});
    }

    Path m_path;
    JobDir m_dir;
    bool m_removeOnDestroy{true};
};

} // namespace job::io