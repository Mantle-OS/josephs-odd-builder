#include "job_egl_surface.h"

namespace job::render {

JobEglSurface::~JobEglSurface()
{
    reset();
}

bool JobEglSurface::create(EGLDisplay display, const JobEglConfig &config, gbm_surface *nativeSurface)
{
    reset();

    if (display == EGL_NO_DISPLAY || !config.valid() || !nativeSurface)
        return false;

    m_surface = eglCreatePlatformWindowSurface(display, config.config(), nativeSurface, nullptr);
    if (m_surface == EGL_NO_SURFACE)
        return false;

    m_display = display;
    m_nativeSurface = nativeSurface;

    if (!refreshSize()) {
        reset();
        return false;
    }

    return true;
}

void JobEglSurface::reset() noexcept
{
    if (m_display != EGL_NO_DISPLAY && m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);

    m_display = EGL_NO_DISPLAY;
    m_surface = EGL_NO_SURFACE;

    m_nativeSurface = nullptr;

    m_width = 0;
    m_height = 0;
}

bool JobEglSurface::swapBuffers() noexcept
{
    if (!valid())
        return false;

    return eglSwapBuffers(m_display, m_surface) == EGL_TRUE;
}

bool JobEglSurface::setSwapInterval(EGLint interval) noexcept
{
    if (!valid())
        return false;

    return eglSwapInterval(m_display, interval) == EGL_TRUE;
}

bool JobEglSurface::refreshSize() noexcept
{
    if (m_display == EGL_NO_DISPLAY || m_surface == EGL_NO_SURFACE)
        return false;

    Width width{};
    Height height{};

    if (eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width) != EGL_TRUE)
        return false;

    if (eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height) != EGL_TRUE)
        return false;

    m_width = width;
    m_height = height;

    return true;
}

}