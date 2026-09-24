#include "job_egl_display.h"

namespace job::render {

JobEglDisplay::~JobEglDisplay()
{
    reset();
}

bool JobEglDisplay::initialize(gbm_device *device)
{
    reset();

    if (!device)
        return false;

    if (!m_clientExtensions.queryClient())
        return false;

    m_display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, device, nullptr);
    if (m_display == EGL_NO_DISPLAY) {
        reset();
        return false;
    }

    if (eglInitialize( m_display, &m_majorVersion, &m_minorVersion) != EGL_TRUE) {
        reset();
        return false;
    }

    m_initialized = true;

    if (!m_displayExtensions.queryDisplay(m_display)) {
        reset();
        return false;
    }

    if (!queryStrings()) {
        reset();
        return false;
    }

    return true;
}

void JobEglDisplay::reset() noexcept
{
    if (m_display != EGL_NO_DISPLAY && m_initialized)
        eglTerminate(m_display);

    m_display = EGL_NO_DISPLAY;
    m_initialized = false;

    m_majorVersion = 0;
    m_minorVersion = 0;

    m_clientExtensions.clear();
    m_displayExtensions.clear();

    m_vendor.clear();
    m_version.clear();
    m_clientApis.clear();
}

bool JobEglDisplay::queryStrings()
{
    if (!valid())
        return false;

    const char *vendor      = eglQueryString(m_display, EGL_VENDOR);
    const char *version     = eglQueryString(m_display, EGL_VERSION);
    const char *clientApis  = eglQueryString(m_display, EGL_CLIENT_APIS);
    if (!vendor || !version || !clientApis)
        return false;

    m_vendor = vendor;
    m_version = version;
    m_clientApis = clientApis;

    return true;
}

}