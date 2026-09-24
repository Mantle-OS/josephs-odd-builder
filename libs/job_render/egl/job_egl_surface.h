#pragma once

#include <cstdint>
#include <memory>

#include <EGL/egl.h>
#include <gbm.h>

#include "job_egl_config.h"

namespace job::render {

class JobEglSurface
{
public:
    using Ptr  = std::shared_ptr<JobEglSurface>;
    using WPtr = std::weak_ptr<JobEglSurface>;
    using UPtr = std::unique_ptr<JobEglSurface>;

    using Width = EGLint;
    using Height = EGLint;

    JobEglSurface() = default;
    ~JobEglSurface();

    JobEglSurface(const JobEglSurface &) = delete;
    JobEglSurface &operator=(const JobEglSurface &) = delete;

    JobEglSurface(JobEglSurface &&) = delete;
    JobEglSurface &operator=(JobEglSurface &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobEglSurface>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobEglSurface>();
    }

    [[nodiscard]] bool create(EGLDisplay display, const JobEglConfig &config, gbm_surface *nativeSurface);
    void reset() noexcept;

    [[nodiscard]] bool swapBuffers() noexcept;

    [[nodiscard]] bool setSwapInterval(EGLint interval) noexcept;

    [[nodiscard]] bool refreshSize() noexcept;

    [[nodiscard]] EGLSurface surface() const noexcept
    {
        return m_surface;
    }

    [[nodiscard]] EGLDisplay display() const noexcept
    {
        return m_display;
    }

    [[nodiscard]] gbm_surface *nativeSurface() noexcept
    {
        return m_nativeSurface;
    }

    [[nodiscard]] const gbm_surface *nativeSurface() const noexcept
    {
        return m_nativeSurface;
    }

    [[nodiscard]] constexpr Width width() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] constexpr Height height() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE && m_nativeSurface != nullptr;
    }

private:
    EGLDisplay      m_display{EGL_NO_DISPLAY};
    EGLSurface      m_surface{EGL_NO_SURFACE};
    gbm_surface     *m_nativeSurface{}; // BORROWED
    Width           m_width{};
    Height          m_height{};
};

}