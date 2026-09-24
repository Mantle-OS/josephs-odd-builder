#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <EGL/egl.h>

#include "job_egl_config.h"

namespace job::render {

enum class EglClientApi : std::uint8_t
{
    OpenGl,
    OpenGlEs
};

class JobEglContext
{
public:
    using Ptr  = std::shared_ptr<JobEglContext>;
    using WPtr = std::weak_ptr<JobEglContext>;
    using UPtr = std::unique_ptr<JobEglContext>;

    using Attribute = EGLint;
    using Attributes = std::vector<Attribute>;

    JobEglContext() = default;
    ~JobEglContext();

    JobEglContext(const JobEglContext &) = delete;
    JobEglContext &operator=(const JobEglContext &) = delete;

    JobEglContext(JobEglContext &&) = delete;
    JobEglContext &operator=(JobEglContext &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEglContext>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEglContext>();
    }

    [[nodiscard]] bool create(EGLDisplay display, const JobEglConfig &config, EglClientApi api, std::span<const Attribute> attributes = {}, EGLContext sharedContext = EGL_NO_CONTEXT);

    void reset() noexcept;

    [[nodiscard]] bool makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) noexcept;
    [[nodiscard]] bool makeCurrent(EGLSurface surface) noexcept
    {
        return makeCurrent(surface, surface);
    }

    [[nodiscard]] bool releaseCurrent() noexcept;

    [[nodiscard]] EGLContext context() const noexcept
    {
        return m_context;
    }

    [[nodiscard]] EGLDisplay display() const noexcept
    {
        return m_display;
    }

    [[nodiscard]] constexpr EglClientApi api() const noexcept
    {
        return m_api;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_display != EGL_NO_DISPLAY && m_context != EGL_NO_CONTEXT;
    }

private:
    [[nodiscard]] static EGLenum eglApi(EglClientApi api) noexcept;

    EGLDisplay      m_display{EGL_NO_DISPLAY};
    EGLContext      m_context{EGL_NO_CONTEXT};
    EglClientApi    m_api{EglClientApi::OpenGl};
};

}