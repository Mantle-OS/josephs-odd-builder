#include "job_egl_context.h"

namespace job::render {

JobEglContext::~JobEglContext()
{
    reset();
}

bool JobEglContext::create(EGLDisplay display, const JobEglConfig &config, EglClientApi api, std::span<const Attribute> attributes, EGLContext sharedContext)
{
    reset();

    if (display == EGL_NO_DISPLAY || !config.valid())
        return false;

    const EGLenum boundApi = eglApi(api);

    if (eglBindAPI(boundApi) != EGL_TRUE)
        return false;

    Attributes contextAttributes;
    contextAttributes.reserve(attributes.size() + 1);

    for (const Attribute attribute : attributes)
        contextAttributes.push_back(attribute);

    if (contextAttributes.empty() || contextAttributes.back() != EGL_NONE)
        contextAttributes.push_back(EGL_NONE);

    m_context = eglCreateContext(display,
                                 config.config(),
                                 sharedContext,
                                 contextAttributes.data());

    if (m_context == EGL_NO_CONTEXT)
        return false;

    m_display = display;
    m_api = api;

    return true;
}

void JobEglContext::reset() noexcept
{
    if (m_display != EGL_NO_DISPLAY && m_context != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_context);

    m_display = EGL_NO_DISPLAY;
    m_context = EGL_NO_CONTEXT;
    m_api = EglClientApi::OpenGl;
}

bool JobEglContext::makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) noexcept
{
    if (!valid())
        return false;

    return eglMakeCurrent(m_display, drawSurface, readSurface, m_context) == EGL_TRUE;
}

bool JobEglContext::releaseCurrent() noexcept
{
    if (m_display == EGL_NO_DISPLAY)
        return false;

    return eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE;
}

EGLenum JobEglContext::eglApi(EglClientApi api) noexcept
{
    switch (api) {
    case EglClientApi::OpenGl:
        return EGL_OPENGL_API;

    case EglClientApi::OpenGlEs:
        return EGL_OPENGL_ES_API;
    }

    return EGL_NONE;
}

}