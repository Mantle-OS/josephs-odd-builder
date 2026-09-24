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

#ifdef JOB_SIMPLE_KMS
    #include "backend/job_kms_simple_backend.h"
#endif

#ifdef JOB_FULL_KMS
    #include "backend/job_kms_backend.h"
#endif

#ifdef JOB_FB
    #include "backend/job_fb_backend.h"
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

#ifdef JOB_SIMPLE_KMS
    [[nodiscard]] JobKmsSimpleBackend *kmsSimpleBackend() noexcept
    {
        if(m_args.backend() != RenderBackend::KmsSimple)
            return nullptr;
        return m_kmsSimpleBackend.get();
    }

    [[nodiscard]] const JobKmsSimpleBackend *kmsSimpleBackend() const noexcept
    {
        if(m_args.backend() != RenderBackend::KmsSimple)
            return nullptr;
        return m_kmsSimpleBackend.get();
    }
#endif

#ifdef JOB_FULL_KMS
    [[nodiscard]] JobKmsBackend *kmsBackend() noexcept
    {
        if(m_args.backend() != RenderBackend::Kms)
            return nullptr;
        return m_kmsBackend.get();
    }

    [[nodiscard]] const JobKmsBackend *kmsBackend() const noexcept
    {
        if(m_args.backend() != RenderBackend::Kms)
            return nullptr;
        return m_kmsBackend.get();
    }
#endif


#ifdef JOB_FB
    [[nodiscard]] JobFbBackend *framebufferBackend() noexcept
    {
        if(m_args.backend() != RenderBackend::Framebuffer)
            return nullptr;
        return m_fbBackend.get();
    }

    [[nodiscard]] const JobFbBackend *framebufferBackend() const noexcept
    {
        if(m_args.backend() != RenderBackend::Framebuffer)
            return nullptr;
        return m_fbBackend.get();
    }
#endif

    void setInputCallback(InputCallback callback);
    void setDeviceCallback(DeviceCallback callback);

private:
    [[nodiscard]] bool startBackend();
    void stopBackend();

#ifdef JOB_SIMPLE_KMS
    [[nodiscard]] bool startSimpleKms();
    void stopSimpleKms();
#endif

#ifdef JOB_FULL_KMS
    [[nodiscard]] bool startKms();
    void stopKms();
#endif

#ifdef JOB_FB
    [[nodiscard]] bool startFramebuffer();
    void stopFramebuffer();
#endif

    void inputEvent(const input::InputEvent &event);
    void deviceChanged();

    JobRenderApplicationArgs        m_args;
    threads::JobIoAsyncThread::Ptr  m_loop;

    input::JobUdevMonitor::Ptr      m_udev;
    input::JobLibinputContext::Ptr  m_input;

#ifdef JOB_SIMPLE_KMS
    JobKmsSimpleBackend::Ptr      m_kmsSimpleBackend;
#endif

#ifdef JOB_FULL_KMS
    JobKmsBackend::Ptr            m_kmsBackend;    
#endif


#ifdef JOB_FB
    JobFbBackend::Ptr       m_fbBackend;
#endif

    InputCallback           m_inputCallback;
    DeviceCallback          m_deviceCallback;

    std::mutex              m_quitMutex;
    std::condition_variable m_quitCondition;

    std::atomic<bool>       m_running{false};
    std::atomic<bool>       m_quitRequested{false};

    int m_exitCode{EXIT_SUCCESS};
};

} // namespace job::render