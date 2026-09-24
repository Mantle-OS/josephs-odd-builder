#include "job_drm_framebuffer_registration.h"

#include <utility>

#include <xf86drmMode.h>

namespace job::render {

JobDrmFramebufferRegistration::~JobDrmFramebufferRegistration()
{
    reset();
}

JobDrmFramebufferRegistration::JobDrmFramebufferRegistration(JobDrmFramebufferRegistration &&other) noexcept :
    m_drmFd(std::exchange(other.m_drmFd, -1)),
    m_framebufferId(std::exchange(other.m_framebufferId, 0))
{
}

JobDrmFramebufferRegistration &JobDrmFramebufferRegistration::operator=(JobDrmFramebufferRegistration &&other) noexcept
{
    if (this == &other)
        return *this;

    reset();

    m_drmFd = std::exchange(other.m_drmFd, -1);
    m_framebufferId = std::exchange(other.m_framebufferId, 0);

    return *this;
}

void JobDrmFramebufferRegistration::reset() noexcept
{
    if (valid())
        drmModeRmFB(m_drmFd, m_framebufferId);

    m_drmFd = -1;
    m_framebufferId = 0;
}

void JobDrmFramebufferRegistration::reset(NativeHandle drmFd, Id framebufferId) noexcept
{
    reset();

    m_drmFd = drmFd;
    m_framebufferId = framebufferId;
}

JobDrmFramebufferRegistration::Id JobDrmFramebufferRegistration::release() noexcept
{
    const Id framebufferId = std::exchange(m_framebufferId, 0);
    m_drmFd = -1;

    return framebufferId;
}

}