#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <gbm.h>

#include "job_egl_extensions.h"

namespace job::render {

class JobEglDisplay
{
public:
    using Ptr  = std::shared_ptr<JobEglDisplay>;
    using WPtr = std::weak_ptr<JobEglDisplay>;
    using UPtr = std::unique_ptr<JobEglDisplay>;

    JobEglDisplay() = default;
    ~JobEglDisplay();

    JobEglDisplay(const JobEglDisplay &) = delete;
    JobEglDisplay &operator=(const JobEglDisplay &) = delete;

    JobEglDisplay(JobEglDisplay &&) = delete;
    JobEglDisplay &operator=(JobEglDisplay &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEglDisplay>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEglDisplay>();
    }

    [[nodiscard]] bool initialize(gbm_device *device);

    void reset() noexcept;

    [[nodiscard]] EGLDisplay display() const noexcept
    {
        return m_display;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_display != EGL_NO_DISPLAY && m_initialized;
    }

    [[nodiscard]] EGLint majorVersion() const noexcept
    {
        return m_majorVersion;
    }

    [[nodiscard]] EGLint minorVersion() const noexcept
    {
        return m_minorVersion;
    }

    [[nodiscard]] const JobEglExtensions &clientExtensions() const noexcept
    {
        return m_clientExtensions;
    }

    [[nodiscard]] const JobEglExtensions &displayExtensions() const noexcept
    {
        return m_displayExtensions;
    }

    [[nodiscard]] std::string_view vendor() const noexcept
    {
        return m_vendor;
    }

    [[nodiscard]] std::string_view version() const noexcept
    {
        return m_version;
    }

    [[nodiscard]] std::string_view clientApis() const noexcept
    {
        return m_clientApis;
    }

private:
    [[nodiscard]] bool queryStrings();

    EGLDisplay          m_display{EGL_NO_DISPLAY};
    bool                m_initialized{};

    EGLint              m_majorVersion{};
    EGLint              m_minorVersion{};

    JobEglExtensions    m_clientExtensions;
    JobEglExtensions    m_displayExtensions;

    std::string         m_vendor;
    std::string         m_version;
    std::string         m_clientApis;
};

}