#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <utility>

namespace job::io {

class JobSysLink
{
public:
    using Ptr  = std::shared_ptr<JobSysLink>;
    using WPtr = std::weak_ptr<JobSysLink>;
    using UPtr = std::unique_ptr<JobSysLink>;

    using Path = std::filesystem::path;

    JobSysLink() = default;
    explicit JobSysLink(Path path);
    ~JobSysLink() = default;

    JobSysLink(const JobSysLink &) = default;
    JobSysLink &operator=(const JobSysLink &) = default;
    JobSysLink(JobSysLink &&) noexcept = default;
    JobSysLink &operator=(JobSysLink &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobSysLink, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobSysLink>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobSysLink, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobSysLink>(std::forward<Args>(args)...);
    }

    [[nodiscard]] const Path &path() const noexcept;
    void setPath(Path path);

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] bool exists() const noexcept;
    [[nodiscard]] bool isSysLink() const noexcept;
    [[nodiscard]] bool isDangling() const noexcept;


    [[nodiscard]] Path target() const;
    [[nodiscard]] Path resolvedTarget() const;
    [[nodiscard]] Path canonicalTarget() const;

    [[nodiscard]] bool create(const Path &target);
    [[nodiscard]] bool createFileLink(const Path &target);
    [[nodiscard]] bool createDirectoryLink(const Path &target);

    [[nodiscard]] bool remove();


    [[nodiscard]] static bool exists(const Path &path) noexcept;
    [[nodiscard]] static bool isSysLink(const Path &path) noexcept;
    [[nodiscard]] static bool isDangling(const Path &path) noexcept;

    [[nodiscard]] static Path target(const Path &path);
    [[nodiscard]] static Path resolvedTarget(const Path &path);
    [[nodiscard]] static Path canonicalTarget(const Path &path);

    [[nodiscard]] static bool create(const Path &path, const Path &target);
    [[nodiscard]] static bool createFileLink(const Path &path, const Path &target);
    [[nodiscard]] static bool createDirectoryLink(const Path &path, const Path &target);

    [[nodiscard]] static bool remove(const Path &path);

private:
    Path m_path;
};

} // namespace job::io