#include "job_ggml.h"

#include "job_ggml_context.h"

namespace job::ggml {

JobGgml::JobGgml(bool scanDevices) :
    m_deviceManager{JobGgmlDeviceManager::createUniq(scanDevices)},
    m_gguf{JobGguf::createUniq()}
{
    ggml_time_init();

    if (const char *value = ggml_version())
        m_version = value;

    if (const char *value = ggml_commit())
        m_commit = value;
}

JobGgml::~JobGgml()
{
    clearLogCallback();
}

bool JobGgml::isValid() const noexcept
{
    return m_deviceManager &&
           m_deviceManager->isValid() &&
           m_gguf;
}

const std::string &JobGgml::version() const noexcept
{
    return m_version;
}

const std::string &JobGgml::commit() const noexcept
{
    return m_commit;
}

JobGgmlDeviceManager *JobGgml::deviceManager() noexcept
{
    return m_deviceManager.get();
}

const JobGgmlDeviceManager *JobGgml::deviceManager() const noexcept
{
    return m_deviceManager.get();
}

JobGguf *JobGgml::gguf() noexcept
{
    return m_gguf.get();
}

const JobGguf *JobGgml::gguf() const noexcept
{
    return m_gguf.get();
}

void JobGgml::setLogCallback(LogCallback callback, void *userData)
{
    m_logCallback = std::move(callback);
    m_logUserData = userData;

    if (m_logCallback)
        ggml_log_set(callLogBouncer, this);
    else
        ggml_log_set(nullptr, nullptr);
}

void JobGgml::clearLogCallback() noexcept
{
    ggml_log_set(nullptr, nullptr);

    m_logCallback = {};
    m_logUserData = nullptr;
}

void JobGgml::timeInit() noexcept
{
    ggml_time_init();
}

std::int64_t JobGgml::timeMs() noexcept
{
    return ggml_time_ms();
}

std::int64_t JobGgml::timeUs() noexcept
{
    return ggml_time_us();
}

std::int64_t JobGgml::cycles() noexcept
{
    return ggml_cycles();
}

std::int64_t JobGgml::cyclesPerMs() noexcept
{
    return ggml_cycles_per_ms();
}

void JobGgml::printObjects(const JobGgmlContext &context)
{
    if (!context.isValid())
        return;

    ggml_print_objects(context.context());
}

void JobGgml::callLogBouncer(ggml_log_level level, const char *text, void *userData)
{
    auto *self = static_cast<JobGgml *>(userData);

    if (!self || !self->m_logCallback)
        return;

    self->m_logCallback(level, text, self->m_logUserData);
}

} // namespace job::ggml