#include "job_fb_backend.h"

#include <job_logger.h>

namespace job::render {

JobFbBackend::~JobFbBackend()
{
    stop();
}

bool JobFbBackend::start(const std::string &device)
{
    if (running())
        return true;

    stop();

    if (device.empty())
        m_device = JobFbDevice::createShared();
    else
        m_device = JobFbDevice::createShared(device);

    if (!m_device || !m_device->openDevice()) {
        JOB_LOG_ERROR("[JobFbBackend] Failed to open framebuffer device.");
        stop();
        return false;
    }

    m_presenter = JobFbPresenter::createShared(m_device.get());

    if (!m_presenter || !m_presenter->valid()) {
        JOB_LOG_ERROR("[JobFbBackend] Failed to initialize framebuffer presenter.");
        stop();
        return false;
    }

    JOB_LOG_INFO("[JobFbBackend] Framebuffer {}x{}, {} bpp.", m_device->width(), m_device->height(), m_device->bitsPerPixel());

    return true;
}

void JobFbBackend::stop() noexcept
{
    if (m_presenter) {
        m_presenter->reset();
        m_presenter.reset();
    }

    if (m_device) {
        m_device->closeDevice();
        m_device.reset();
    }
}

bool JobFbBackend::refresh()
{
    if (!m_presenter)
        return false;

    if (!m_presenter->refresh()) {
        JOB_LOG_ERROR("[JobFbBackend] Failed to refresh framebuffer state.");
        return false;
    }

    return true;
}

JobFbBuffer JobFbBackend::framebuffer() noexcept
{
    if (!m_presenter)
        return {};

    return m_presenter->framebuffer();
}

}