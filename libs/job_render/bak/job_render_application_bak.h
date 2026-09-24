#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>

#include <job_io_async_thread.h>

#include "input/job_input_event.h"
#include "input/job_libinput_context.h"
#include "input/job_udev_monitor.h"

#include "job_render_application_args.h"

#if defined(JOB_RENDER_HAS_KMS)
#include "kms/job_drm_device.h"
#include "kms/job_kms_presenter.h"
#include "kms/job_kms_surface.h"
#endif

#if defined(JOB_RENDER_HAS_FB)
#include "fb/job_fb_device.h"
#include "fb/job_fb_presenter.h"
#endif

#include "jobrender_export.h"
namespace job::render {

class JOBRENDER_EXPORT JobRenderApplication
{
public:
    using Ptr  = std::shared_ptr<JobRenderApplication>;
    using WPtr = std::weak_ptr<JobRenderApplication>;
    using UPtr = std::unique_ptr<JobRenderApplication>;

    using InputCallback  = std::function<void(const input::InputEvent &)>;
    using DeviceCallback = std::function<void()>;

    JobRenderApplication(int argc, char *argv[]);
    ~JobRenderApplication();

    JobRenderApplication(const JobRenderApplication &) = delete;
    JobRenderApplication &operator=(const JobRenderApplication &) = delete;
    JobRenderApplication(JobRenderApplication &&) = delete;
    JobRenderApplication &operator=(JobRenderApplication &&) = delete;

    [[nodiscard]] static Ptr createShared(int argc, char *argv[]);
    [[nodiscard]] static UPtr createUniq(int argc, char *argv[]);

    [[nodiscard]] bool start();
    void stop();

    [[nodiscard]] int exec();
    int quit(int result = EXIT_SUCCESS);

    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] const JobRenderApplicationArgs &args() const noexcept;
    [[nodiscard]] threads::JobIoAsyncThread::Ptr eventLoop() const noexcept;

    void setInputCallback(InputCallback callback);
    void setDeviceCallback(DeviceCallback callback);

private:
    [[nodiscard]] bool startBackend();
    void stopBackend();

#if defined(JOB_RENDER_HAS_KMS)
    [[nodiscard]] bool startKms();
    void stopKms();
#endif

#if defined(JOB_RENDER_HAS_FB)
    [[nodiscard]] bool startFramebuffer();
    void stopFramebuffer();
#endif

    void inputEvent(const input::InputEvent &event);
    void deviceChanged();

    JobRenderApplicationArgs m_args;

    threads::JobIoAsyncThread::Ptr m_loop;

    input::JobUdevMonitor::Ptr m_udev;
    input::JobLibinputContext::Ptr m_input;

#if defined(JOB_RENDER_HAS_KMS)
    JobDrmDevice::Ptr m_drmDevice;
    JobKmsPresenter::Ptr m_kmsPresenter;
    JobKmsSurface::Ptr m_kmsSurface;
#endif


    JobFbDevice::Ptr m_fbDevice;
    JobFbPresenter::Ptr m_fbPresenter;


    InputCallback m_inputCallback;
    DeviceCallback m_deviceCallback;

    std::mutex m_quitMutex;
    std::condition_variable m_quitCondition;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_quitRequested{false};

    int m_exitCode{EXIT_SUCCESS};
};

} // namespace job::render