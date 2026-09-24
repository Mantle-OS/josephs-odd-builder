#include "job_render_application.h"

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
        return m_running;

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
    if (!m_udev || !m_udev->start(m_loop, [this]() { deviceChanged(); })) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start udev monitor.");
        stop();
        return false;
    }

    m_input = input::JobLibinputContext::createShared();
    if (!m_input || !m_input->start(m_loop, [this](const input::InputEvent &event) { inputEvent(event); }, m_args.seat())) {
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
    return m_running;
}

void JobRenderApplication::stop()
{
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
    m_quitCondition.wait(lock, [this]() { return m_quitRequested.load(); });
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
#ifdef JOB_FULL_KMS
        return startKms();
#else
        JOB_LOG_ERROR("[JobRenderApplication] Large KMS backend was not built.");
        return false;
#endif

    case RenderBackend::KmsSimple:
#ifdef JOB_SIMPLE_KMS
        return startSimpleKms();
#else
        JOB_LOG_ERROR("[JobRenderApplication] Simple KMS backend was not built.");
        return false;
#endif

    case RenderBackend::Framebuffer:
#ifdef JOB_FB
        return startFramebuffer();
#else
        JOB_LOG_ERROR("[JobRenderApplication] Framebuffer backend was not built.");
        return false;
#endif
    case RenderBackend::Default:
        JOB_LOG_ERROR("[JobRenderApplication] Default render will come later Not sure how you got to this point lol.");
        return false;
    }
    return false;
}

void JobRenderApplication::stopBackend()
{
    auto type = m_args.backend();
    if(type == RenderBackend::Default){
        JOB_LOG_ERROR( "[JobRenderApplication] Default is not ready stop stopping lol");
        return;
    }
#ifdef JOB_SIMPLE_KMS
    else if (type == RenderBackend::KmsSimple) {
        stopSimpleKms();
    }
#endif

#ifdef JOB_FULL_KMS
    else if(type == RenderBackend::Kms){
        stopKms();
    }
#endif

#ifdef JOB_FB
    else if (type == RenderBackend::Framebuffer) {
        stopFramebuffer();
    }
#endif    
    else {
        JOB_LOG_ERROR( "[JobRenderApplication] I have no clue what backend you are ... or how to stop the type is unknown");
    }
}


#ifdef JOB_SIMPLE_KMS
bool JobRenderApplication::startSimpleKms()
{
    if(m_args.backend() != RenderBackend::KmsSimple){
        JOB_LOG_ERROR("[JobRenderApplication] trying to start Simple KMS backend with the wrong argument type.");
        return false;
    }

    if (m_kmsSimpleBackend && m_kmsSimpleBackend->running())
        return true;

    stopSimpleKms();

    if(!m_loop || !m_loop->isRunning()){
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create Simple KMS backend. Async Event loop is not valid or is not running");
        return false;
    }

    m_kmsSimpleBackend = JobKmsSimpleBackend::createShared();

    if (!m_kmsSimpleBackend) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create Simple KMS backend.");
        return false;
    }


    if (!m_kmsSimpleBackend->start(m_loop, m_args.device())) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start Simple KMS backend.");
        stopSimpleKms();
        return false;
    }

    JOB_LOG_INFO( "[JobRenderApplication] KMS Simple backend ready at {}x{}.", m_kmsSimpleBackend->width(), m_kmsSimpleBackend->height());

    return true;

}
void JobRenderApplication::stopSimpleKms()
{
    if(m_args.backend() != RenderBackend::KmsSimple){
        JOB_LOG_ERROR("[JobRenderApplication] trying to stop Simple KMS backend with the wrong argument type.");
        return;
    }

    if (!m_kmsSimpleBackend)
        return;

    m_kmsSimpleBackend->stop();
    m_kmsSimpleBackend.reset();
}
#endif

#ifdef JOB_FULL_KMS
bool JobRenderApplication::startKms()
{
    if(m_args.backend() != RenderBackend::Kms){
        JOB_LOG_ERROR("[JobRenderApplication] trying to start KMS backend with the wrong argument type.");
        return false;
    }

    if (m_kmsBackend && m_kmsBackend->running())
        return true;

    stopKms();

    m_kmsBackend = JobKmsBackend::createShared();

    if (!m_kmsBackend) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create KMS backend.");
        return false;
    }

    if (!m_kmsBackend->start(m_args.device())) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start KMS backend.");
        stopKms();
        return false;
    }

    JOB_LOG_INFO( "[JobRenderApplication] KMS backend ready at {}x{}.", m_kmsBackend->width(), m_kmsBackend->height() );

    return true;
}

void JobRenderApplication::stopKms()
{
    if(m_args.backend() != RenderBackend::Kms){
        JOB_LOG_ERROR("[JobRenderApplication] trying to stop KMS backend with the wrong argument type.");
        return;
    }

    if (!m_kmsBackend)
        return;

    m_kmsBackend->stop();
    m_kmsBackend.reset();
}
#endif

#ifdef JOB_FB

bool JobRenderApplication::startFramebuffer()
{
    if (m_fbBackend && m_fbBackend->running())
        return true;

    stopFramebuffer();

    m_fbBackend = JobFbBackend::createShared();

    if (!m_fbBackend) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to create framebuffer backend.");
        return false;
    }

    if (!m_fbBackend->start(m_args.device())) {
        JOB_LOG_ERROR("[JobRenderApplication] Failed to start framebuffer backend.");
        stopFramebuffer();
        return false;
    }

    JOB_LOG_INFO("[JobRenderApplication] Framebuffer backend ready.");

    return true;
}

void JobRenderApplication::stopFramebuffer()
{
    if (!m_fbBackend)
        return;

    m_fbBackend->stop();
    m_fbBackend.reset();
}

#endif

void JobRenderApplication::inputEvent(const input::InputEvent &event)
{
    if (m_inputCallback)
        m_inputCallback(event);
}

void JobRenderApplication::deviceChanged()
{
    /*
     * JobUdevMonitor currently collapses input, DRM, USB, and TTY changes
     * into the same callback. Do not rebuild the active render backend here
     * until the monitor tells us which subsystem actually changed.
     */

    if (m_deviceCallback)
        m_deviceCallback();
}

} // namespace job::render