#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace job::render {

class JobEglExtensions
{
public:
    using Ptr  = std::shared_ptr<JobEglExtensions>;
    using WPtr = std::weak_ptr<JobEglExtensions>;
    using UPtr = std::unique_ptr<JobEglExtensions>;

    using Extensions = std::unordered_set<std::string>;

    JobEglExtensions() = default;
    ~JobEglExtensions() = default;

    JobEglExtensions(const JobEglExtensions &) = default;
    JobEglExtensions &operator=(const JobEglExtensions &) = default;

    JobEglExtensions(JobEglExtensions &&) noexcept = default;
    JobEglExtensions &operator=(JobEglExtensions &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEglExtensions>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEglExtensions>();
    }

    [[nodiscard]] bool queryClient();

    [[nodiscard]] bool queryDisplay(EGLDisplay display);

    [[nodiscard]] bool contains(std::string_view extension) const
    {
        return m_extensions.contains(std::string{extension});
    }

    [[nodiscard]] const Extensions &extensions() const noexcept
    {
        return m_extensions;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_extensions.empty();
    }

    [[nodiscard]] std::size_t count() const noexcept
    {
        return m_extensions.size();
    }

    void clear() noexcept
    {
        m_extensions.clear();
    }

private:
    [[nodiscard]] bool query(EGLDisplay display);
    Extensions  m_extensions;
};

}