#include "job_render_application.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include <job_logger.h>

namespace job::render {

JobRenderApplication::JobRenderApplication(int argc, char *argv[]) :
    m_args(argc, argv)
{
}

JobRenderApplication::~JobRenderApplication()
{
    stop();
}

JobRenderApplication::Ptr JobRenderApplication::createShared(int argc, char *argv[])
{
    return std::make_shared<JobRenderApplication>(argc, argv);
}

JobRenderApplication::UPtr JobRenderApplication::createUniq(int argc, char *argv[])
{
    return std::make_unique<JobRenderApplication>(argc, argv);
}

bool JobRenderApplication::start()
{
    if (m_running)
        return true;

    if (!m_args.valid()) {
        JOB_LOG_ERROR("[JobRenderApplication] {}", m_args.error());
        return false;
    }

    if (m_args.help()) {
        std::cout << JobRenderApplicationArgs::helpText();
        m_exitCode = EXIT_SUCCESS;
        m_quitRequested = true;
        return true;
    }

    if (m_args.version()) {
        std::cout << JobRenderApplicationArgs::versionText() << '\n';
        m_exitCode = EXIT_SUCCESS;
        m_quitRequested = true;
        return true;
    }

    m_quitRequested = false;
    m_exitCode = EXIT_SUCCESS;

    m_loop = threads::JobIoAsyncThread::createShared();

    if (!m_loop) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create I/O event loop.");
        stop();
        return false;
    }

    m_loop->start();

    if (!m_loop->isRunning()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start I/O event loop.");
        stop();
        return false;
    }

    m_udev = input::JobUdevMonitor::createShared();

    if (!m_udev || !m_udev->start(m_loop, [this]() {
            deviceChanged();
        })) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start udev monitor.");
        stop();
        return false;
    }

    m_input = input::JobLibinputContext::createShared();

    if (!m_input || !m_input->start(m_loop, [this](const input::InputEvent &event) {
            inputEvent(event);
        })) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start libinput.");
        stop();
        return false;
    }

    if (!startBackend()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start render backend.");
        stop();
        return false;
    }

    m_running = true;
    return true;
}

void JobRenderApplication::stop()
{
    if (!m_loop && !m_udev && !m_input) {
        m_running = false;
        return;
    }

    m_running = false;

    stopBackend();

    if (m_input) {
        m_input->stop();
        m_input.reset();
    }

    if (m_udev) {
        m_udev->stop();
        m_udev.reset();
    }

    if (m_loop) {
        m_loop->stop();
        m_loop.reset();
    }
}

int JobRenderApplication::exec()
{
    if (m_quitRequested) {
        stop();
        return m_exitCode;
    }

    std::unique_lock lock(m_quitMutex);

    m_quitCondition.wait(lock, [this]() {
        return m_quitRequested.load();
    });

    const int result = m_exitCode;

    lock.unlock();

    stop();

    return result;
}

int JobRenderApplication::quit(int result)
{
    {
        std::lock_guard lock(m_quitMutex);

        if (!m_quitRequested) {
            m_exitCode = result;
            m_quitRequested = true;
        }
    }

    m_quitCondition.notify_all();

    return result;
}

bool JobRenderApplication::running() const noexcept
{
    return m_running;
}

const JobRenderApplicationArgs &JobRenderApplication::args() const noexcept
{
    return m_args;
}

threads::JobIoAsyncThread::Ptr JobRenderApplication::eventLoop() const noexcept
{
    return m_loop;
}

void JobRenderApplication::setInputCallback(InputCallback callback)
{
    m_inputCallback = std::move(callback);
}

void JobRenderApplication::setDeviceCallback(DeviceCallback callback)
{
    m_deviceCallback = std::move(callback);
}

bool JobRenderApplication::startBackend()
{
    switch (m_args.backend()) {
    case RenderBackend::Kms:
#if defined(JOB_RENDER_HAS_KMS)
        return startKms();
#else
        JOB_LOG_ERROR("[JobRenderApplication] KMS backend was not built.");
        return false;
#endif

    case RenderBackend::Framebuffer:
#if defined(JOB_RENDER_HAS_FB)
        return startFramebuffer();
#else
        JOB_LOG_ERROR("[JobRenderApplication] Framebuffer backend was not built.");
        return false;
#endif

    case RenderBackend::Default:
        break;
    }

#if defined(JOB_RENDER_HAS_KMS) && !defined(JOB_RENDER_HAS_FB)
    return startKms();
#elif defined(JOB_RENDER_HAS_FB) && !defined(JOB_RENDER_HAS_KMS)
    return startFramebuffer();
#elif defined(JOB_RENDER_HAS_KMS) && defined(JOB_RENDER_HAS_FB)
    JOB_LOG_ERROR(
        "[JobRenderApplication] Multiple render backends are available. "
        "Select one explicitly with --backend."
        );
    return false;
#else
    JOB_LOG_ERROR("[JobRenderApplication] No render backend was built.");
    return false;
#endif
}

void JobRenderApplication::stopBackend()
{
#if defined(JOB_RENDER_HAS_KMS)
    stopKms();
#endif

#if defined(JOB_RENDER_HAS_FB)
    stopFramebuffer();
#endif
}

#if defined(JOB_RENDER_HAS_KMS)

bool JobRenderApplication::startKms()
{
    if (m_args.device().empty())
        m_drmDevice = JobDrmDevice::createShared();
    else
        m_drmDevice = JobDrmDevice::createShared(m_args.device());

    if (!m_drmDevice || !m_drmDevice->openDevice()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to open DRM device.");
        stopKms();
        return false;
    }

    m_kmsPresenter = JobKmsPresenter::createShared(m_drmDevice);

    if (!m_kmsPresenter || !m_kmsPresenter->start(m_loop)) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start KMS presenter.");
        stopKms();
        return false;
    }

    m_kmsSurface = JobKmsSurface::createShared(m_drmDevice);

    if (!m_kmsSurface ||
        !m_kmsSurface->create(
            m_kmsPresenter->width(),
            m_kmsPresenter->height(),
            RenderFormat::XRGB8888)) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create KMS surface.");
        stopKms();
        return false;
    }

    std::ranges::fill(m_kmsSurface->data(), std::byte{0xFF});

    if (!m_kmsPresenter->present(m_kmsSurface)) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to present initial KMS surface.");
        stopKms();
        return false;
    }

    JOB_LOG_INFO(
        "[JobRenderApplication] KMS {}x{} {} Hz.",
        m_kmsPresenter->width(),
        m_kmsPresenter->height(),
        m_kmsPresenter->refreshRate()
        );

    return true;
}

void JobRenderApplication::stopKms()
{
    if (m_kmsPresenter) {
        m_kmsPresenter->stop();
        m_kmsPresenter.reset();
    }

    if (m_kmsSurface) {
        m_kmsSurface->destroy();
        m_kmsSurface.reset();
    }

    if (m_drmDevice) {
        m_drmDevice->closeDevice();
        m_drmDevice.reset();
    }
}

#endif

#if defined(JOB_RENDER_HAS_FB)

bool JobRenderApplication::startFramebuffer()
{
    if (m_args.device().empty())
        m_fbDevice = JobFbDevice::createShared();
    else
        m_fbDevice = JobFbDevice::createShared(m_args.device());

    if (!m_fbDevice || !m_fbDevice->openDevice()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to open framebuffer device.");
        stopFramebuffer();
        return false;
    }

    m_fbPresenter = JobFbPresenter::createShared(m_fbDevice);

    if (!m_fbPresenter || !m_fbPresenter->start()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start framebuffer presenter.");
        stopFramebuffer();
        return false;
    }

    std::ranges::fill(m_fbDevice->data(), std::byte{0xFF});

    if (!m_fbPresenter->present()) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to present framebuffer.");
        stopFramebuffer();
        return false;
    }

    JOB_LOG_INFO(
        "[JobRenderApplication] Framebuffer {}x{}, {} bpp.",
        m_fbDevice->width(),
        m_fbDevice->height(),
        m_fbDevice->bitsPerPixel()
        );

    return true;
}

void JobRenderApplication::stopFramebuffer()
{
    if (m_fbPresenter) {
        m_fbPresenter->stop();
        m_fbPresenter.reset();
    }

    if (m_fbDevice) {
        m_fbDevice->closeDevice();
        m_fbDevice.reset();
    }
}

#endif

void JobRenderApplication::inputEvent(const input::InputEvent &event)
{
    if (m_inputCallback)
        m_inputCallback(event);
}

void JobRenderApplication::deviceChanged()
{
#if defined(JOB_RENDER_HAS_KMS)
    if (m_drmDevice && m_kmsPresenter) {
        if (!m_kmsPresenter->presentPending()) {
            if (!m_kmsPresenter->refresh())
                JOB_LOG_WARN("[JobRenderApplication] Failed to refresh KMS topology.");
        }
    }
#endif

#if defined(JOB_RENDER_HAS_FB)
    if (m_fbDevice && m_fbPresenter) {
        if (!m_fbPresenter->refresh())
            JOB_LOG_WARN("[JobRenderApplication] Failed to refresh framebuffer state.");
    }
#endif

    if (m_deviceCallback)
        m_deviceCallback();
}

} // namespace job::render