#pragma once

#include <memory>
#include <string>

#include <job_io_async_thread.h>

#include "kms_simple/job_drm_simple_device.h"
#include "kms_simple/job_kms_simple_presenter.h"
#include "kms_simple/job_kms_simple_surface.h"

namespace job::render {

class JobKmsSimpleBackend
{
public:
    using Ptr  = std::shared_ptr<JobKmsSimpleBackend>;
    using WPtr = std::weak_ptr<JobKmsSimpleBackend>;
    using UPtr = std::unique_ptr<JobKmsSimpleBackend>;

    JobKmsSimpleBackend() = default;
    ~JobKmsSimpleBackend();

    JobKmsSimpleBackend(const JobKmsSimpleBackend &) = delete;
    JobKmsSimpleBackend &operator=(const JobKmsSimpleBackend &) = delete;

    JobKmsSimpleBackend(JobKmsSimpleBackend &&) noexcept = delete;
    JobKmsSimpleBackend &operator=(JobKmsSimpleBackend &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobKmsSimpleBackend>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobKmsSimpleBackend>();
    }

    [[nodiscard]] bool start(const threads::JobIoAsyncThread::Ptr &loop, const std::string &device = {}, FormatType format = FormatType::XRGB8888);

    void stop() noexcept;

    [[nodiscard]] bool refresh();

    [[nodiscard]] bool present();

    [[nodiscard]] JobDrmSimpleDevice *device() noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] const JobDrmSimpleDevice *device() const noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] JobKmsSimplePresenter *presenter() noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] const JobKmsSimplePresenter *presenter() const noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] JobKmsSimpleSurface *surface() noexcept
    {
        return m_surface.get();
    }

    [[nodiscard]] const JobKmsSimpleSurface *surface() const noexcept
    {
        return m_surface.get();
    }

    [[nodiscard]] std::uint32_t width() const noexcept
    {
        return m_presenter ? m_presenter->width() : 0;
    }

    [[nodiscard]] std::uint32_t height() const noexcept
    {
        return m_presenter ? m_presenter->height() : 0;
    }

    [[nodiscard]] std::uint32_t refreshRate() const noexcept
    {
        return m_presenter ? m_presenter->refreshRate() : 0;
    }

    [[nodiscard]] bool running() const noexcept
    {
        return m_device &&
               m_device->isOpen() &&
               m_presenter &&
               m_presenter->running() &&
               m_surface &&
               m_surface->valid();
    }

private:
    JobDrmSimpleDevice::Ptr     m_device;
    JobKmsSimplePresenter::Ptr  m_presenter;
    JobKmsSimpleSurface::Ptr    m_surface;
};

} // namespace job::render