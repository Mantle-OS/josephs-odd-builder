#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "job_drm_connector.h"
#include "job_drm_crtc.h"
#include "job_drm_device.h"
#include "job_drm_plane.h"

#include "job_egl_config.h"
#include "job_egl_context.h"
#include "job_egl_display.h"
#include "job_egl_surface.h"

#include "job_gbm_device.h"
#include "job_gbm_surface.h"

#include "job_kms_presenter.h"

namespace job::render {

class JobKmsBackend
{
public:
    using Ptr  = std::shared_ptr<JobKmsBackend>;
    using WPtr = std::weak_ptr<JobKmsBackend>;
    using UPtr = std::unique_ptr<JobKmsBackend>;

    JobKmsBackend() = default;
    ~JobKmsBackend();

    JobKmsBackend(const JobKmsBackend &) = delete;
    JobKmsBackend &operator=(const JobKmsBackend &) = delete;

    JobKmsBackend(JobKmsBackend &&) noexcept = delete;
    JobKmsBackend &operator=(JobKmsBackend &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobKmsBackend>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobKmsBackend>();
    }

    [[nodiscard]] bool start(const std::string &device = {});
    void stop() noexcept;

    [[nodiscard]] bool refresh();
    [[nodiscard]] bool present();

    [[nodiscard]] JobDrmDevice *drmDevice() noexcept
    {
        return m_drmDevice.get();
    }

    [[nodiscard]] const JobDrmDevice *drmDevice() const noexcept
    {
        return m_drmDevice.get();
    }

    [[nodiscard]] JobGbmDevice *gbmDevice() noexcept
    {
        return m_gbmDevice.get();
    }

    [[nodiscard]] const JobGbmDevice *gbmDevice() const noexcept
    {
        return m_gbmDevice.get();
    }

    [[nodiscard]] JobGbmSurface *gbmSurface() noexcept
    {
        return m_gbmSurface.get();
    }

    [[nodiscard]] const JobGbmSurface *gbmSurface() const noexcept
    {
        return m_gbmSurface.get();
    }

    [[nodiscard]] JobEglDisplay *eglDisplay() noexcept
    {
        return m_eglDisplay.get();
    }

    [[nodiscard]] const JobEglDisplay *eglDisplay() const noexcept
    {
        return m_eglDisplay.get();
    }

    [[nodiscard]] JobEglConfig *eglConfig() noexcept
    {
        return m_eglConfig.get();
    }

    [[nodiscard]] const JobEglConfig *eglConfig() const noexcept
    {
        return m_eglConfig.get();
    }

    [[nodiscard]] JobEglContext *eglContext() noexcept
    {
        return m_eglContext.get();
    }

    [[nodiscard]] const JobEglContext *eglContext() const noexcept
    {
        return m_eglContext.get();
    }

    [[nodiscard]] JobEglSurface *eglSurface() noexcept
    {
        return m_eglSurface.get();
    }

    [[nodiscard]] const JobEglSurface *eglSurface() const noexcept
    {
        return m_eglSurface.get();
    }

    [[nodiscard]] JobKmsPresenter *presenter() noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] const JobKmsPresenter *presenter() const noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] std::uint32_t width() const noexcept
    {
        return m_mode.hdisplay();
    }

    [[nodiscard]] std::uint32_t height() const noexcept
    {
        return m_mode.vdisplay();
    }

    [[nodiscard]] const JobDrmMode &mode() const noexcept
    {
        return m_mode;
    }

    [[nodiscard]] bool running() const noexcept
    {
        return m_drmDevice &&
               m_gbmDevice &&
               m_gbmSurface &&
               m_eglDisplay &&
               m_eglConfig &&
               m_eglContext &&
               m_eglSurface &&
               m_presenter;
    }

private:
    [[nodiscard]] bool discoverDisplay();
    [[nodiscard]] bool createGbm();
    [[nodiscard]] bool createEgl();
    [[nodiscard]] bool createPresenter();

    [[nodiscard]] static const JobDrmMode *selectMode(const JobDrmConnector &connector) noexcept;
    [[nodiscard]] static bool primaryPlane(const JobDrmPlane &plane) noexcept;

    void resetDisplayState() noexcept;

    JobDrmDevice::Ptr m_drmDevice;

    JobDrmConnector m_connector;
    JobDrmCrtc      m_crtc;
    JobDrmPlane     m_plane;
    JobDrmMode      m_mode;

    JobDrmFormat m_format;
    bool         m_explicitModifiers{};

    JobGbmDevice::Ptr  m_gbmDevice;
    JobGbmSurface::Ptr m_gbmSurface;

    JobEglDisplay::Ptr m_eglDisplay;
    JobEglConfig::Ptr  m_eglConfig;
    JobEglContext::Ptr m_eglContext;
    JobEglSurface::Ptr m_eglSurface;

    JobKmsPresenter::Ptr m_presenter;

    std::uint64_t m_nextFrameId{1};
};

} // namespace job::render