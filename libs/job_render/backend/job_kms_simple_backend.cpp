#include "job_kms_simple_backend.h"

#include <utility>

#include <job_logger.h>

namespace job::render {

JobKmsSimpleBackend::~JobKmsSimpleBackend()
{
    stop();
}

bool JobKmsSimpleBackend::start(const threads::JobIoAsyncThread::Ptr &loop, const std::string &device, core::FormatType format)
{
    if (running())
        return true;

    stop();

    m_device = device.empty() ? JobDrmSimpleDevice::createShared() : JobDrmSimpleDevice::createShared(device);
    if (!m_device || !m_device->openDevice()) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to open DRM device.");
        stop();
        return false;
    }

    m_presenter = JobKmsSimplePresenter::createShared(m_device);

    if (!m_presenter || !m_presenter->start(loop)) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to start KMS presenter.");
        stop();
        return false;
    }

    m_surface = JobKmsSimpleSurface::createShared(m_device);

    if (!m_surface || !m_surface->create( m_presenter->width(), m_presenter->height(), format)) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to create KMS surface.");
        stop();
        return false;
    }

    if (!m_presenter->present(m_surface)) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to present initial KMS surface.");
        stop();
        return false;
    }

    JOB_LOG_INFO("[JobKmsSimpleBackend] KMS {}x{} {} Hz.", width(), height(), refreshRate());
    return true;
}

void JobKmsSimpleBackend::stop() noexcept
{
    if (m_presenter) {
        m_presenter->stop();
        m_presenter.reset();
    }

    if (m_surface) {
        m_surface->destroy();
        m_surface.reset();
    }

    if (m_device) {
        m_device->closeDevice();
        m_device.reset();
    }
}

bool JobKmsSimpleBackend::refresh()
{
    if (!running())
        return false;

    if (m_presenter->presentPending())
        return false;

    const core::FormatType format = m_surface->format();

    if (!m_presenter->refresh())
        return false;

    m_surface->destroy();

    if (!m_surface->create(width(), height(), format)) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to recreate KMS surface after refresh.");
        return false;
    }

    if (!m_presenter->present(m_surface)) {
        JOB_LOG_ERROR("[JobKmsSimpleBackend] Failed to present KMS surface after refresh.");
        return false;
    }

    return true;
}

bool JobKmsSimpleBackend::present()
{
    if (!running())
        return false;

    return m_presenter->present(m_surface);
}

} // namespace job::render