#include "job_fb_presenter.h"

namespace job::render {

JobFbPresenter::JobFbPresenter(JobFbDevice *device) noexcept
{
    (void)initialize(device);
}

bool JobFbPresenter::initialize(JobFbDevice *device) noexcept
{
    reset();

    if (!device || !device->valid() || !device->mapped())
        return false;

    m_device = device;
    return true;
}

void JobFbPresenter::reset() noexcept
{
    m_device = nullptr;
}

JobFbBuffer JobFbPresenter::framebuffer() noexcept
{
    if (!valid())
        return {};

    return JobFbBuffer{
        m_device->data(),
        m_device->mappedSize(),
        m_device->width(),
        m_device->height(),
        m_device->virtualWidth(),
        m_device->virtualHeight(),
        m_device->xOffset(),
        m_device->yOffset(),
        m_device->stride(),
        m_device->bitsPerPixel(),
        m_device->format(),
        m_device->fixedInfo().visual()
    };
}

bool JobFbPresenter::refresh() noexcept
{
    if (!m_device || !m_device->isOpen())
        return false;

    m_device->unmap();

    if (!m_device->refresh())
        return false;

    if (!m_device->map())
        return false;

    return true;
}

}