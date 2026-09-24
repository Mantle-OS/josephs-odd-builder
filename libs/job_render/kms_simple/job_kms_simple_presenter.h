#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include <xf86drmMode.h>

#include <job_io_async_thread.h>

#include "job_drm_simple_device.h"
#include "job_kms_simple_surface.h"

namespace job::render {

class JobKmsSimplePresenter
{
public:
    using Ptr  = std::shared_ptr<JobKmsSimplePresenter>;
    using WPtr = std::weak_ptr<JobKmsSimplePresenter>;
    using UPtr = std::unique_ptr<JobKmsSimplePresenter>;

    using Loop = std::weak_ptr<threads::JobIoAsyncThread>;

    using PresentCallback = std::function<void()>;

    explicit JobKmsSimplePresenter(JobDrmSimpleDevice::Ptr device);
    ~JobKmsSimplePresenter();

    JobKmsSimplePresenter(const JobKmsSimplePresenter &) = delete;
    JobKmsSimplePresenter &operator=(const JobKmsSimplePresenter &) = delete;
    JobKmsSimplePresenter(JobKmsSimplePresenter &&) = delete;
    JobKmsSimplePresenter &operator=(JobKmsSimplePresenter &&) = delete;

    [[nodiscard]] static Ptr createShared(JobDrmSimpleDevice::Ptr device);
    [[nodiscard]] static UPtr createUniq(JobDrmSimpleDevice::Ptr device);

    [[nodiscard]] bool start(threads::JobIoAsyncThread::Ptr loop);
    void stop();

    [[nodiscard]] bool refresh();

    [[nodiscard]] bool present(const JobKmsSimpleSurface::Ptr &surface);
    [[nodiscard]] bool presentPending() const noexcept;

    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] std::uint32_t connectorId() const noexcept;
    [[nodiscard]] std::uint32_t encoderId() const noexcept;
    [[nodiscard]] std::uint32_t crtcId() const noexcept;

    [[nodiscard]] const drmModeModeInfo &mode() const noexcept;

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t refreshRate() const noexcept;

    void setPresentCallback(PresentCallback callback);

private:
    struct DrmModeConnectorDeleter
    {
        void operator()(drmModeConnector *connector) const noexcept;
    };

    struct DrmModeEncoderDeleter
    {
        void operator()(drmModeEncoder *encoder) const noexcept;
    };

    struct DrmModeCrtcDeleter
    {
        void operator()(drmModeCrtc *crtc) const noexcept;
    };

    using DrmModeConnectorPtr = std::unique_ptr<drmModeConnector, DrmModeConnectorDeleter>;
    using DrmModeEncoderPtr   = std::unique_ptr<drmModeEncoder, DrmModeEncoderDeleter>;
    using DrmModeCrtcPtr      = std::unique_ptr<drmModeCrtc, DrmModeCrtcDeleter>;

    [[nodiscard]] bool findDisplay();
    [[nodiscard]] bool findConnector();
    [[nodiscard]] bool findEncoder();
    [[nodiscard]] bool findCrtc();
    [[nodiscard]] bool findMode();

    [[nodiscard]] bool modeset(const JobKmsSimpleSurface::Ptr &surface);
    [[nodiscard]] bool pageFlip(const JobKmsSimpleSurface::Ptr &surface);

    void restore();

    void onEvents(threads::IOEvent events);
    void handleEvents();

    static void pageFlipHandler(int fd, unsigned int sequence, unsigned int tvSec, unsigned int tvUsec, void *userData);

    void pageFlipped();

    JobDrmSimpleDevice::Ptr m_device;
    Loop m_loop;

    DrmModeConnectorPtr m_connector;
    DrmModeEncoderPtr   m_encoder;
    DrmModeCrtcPtr      m_savedCrtc;

    drmModeModeInfo     m_mode{};

    std::uint32_t m_connectorId{};
    std::uint32_t m_encoderId{};
    std::uint32_t m_crtcId{};

    JobKmsSimpleSurface::Ptr m_current;
    JobKmsSimpleSurface::Ptr m_pending;

    PresentCallback m_presentCallback;

    bool m_modeset{false};
    bool m_presentPending{false};
    bool m_running{false};
};

} // namespace job::render