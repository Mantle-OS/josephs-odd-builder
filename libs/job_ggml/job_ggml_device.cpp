#include "job_ggml_device.h"

#include <job_logger.h>
#include <stdexcept>

namespace job::ggml {

JobGgmlDevice::JobGgmlDevice(ggml_backend_dev_t dev):
    m_dev(dev)
{
    // Cache device properties — no need to hit the registry every time
    ggml_backend_dev_get_props(m_dev, &m_props);
    m_name = m_props.name ? m_props.name : "unnamed";

    // Initialize the backend — this is what actually gives us the compute unit
    m_backend = ggml_backend_dev_init(m_dev, nullptr);
    if (!m_backend) {
        JOB_LOG_ERROR("[JobGgmlDevice] Failed to initialize backend for '{}'", m_name);
        throw std::runtime_error("Failed to initialize backend for device: " + m_name);
    }

    // Cache buffer types — these are what we'll use when creating tensors
    m_bufferType = ggml_backend_dev_buffer_type(m_dev);
    m_hostBufferType = ggml_backend_dev_host_buffer_type(m_dev);

    size_t freeMem = 0;
    size_t totalMem = 0;
    ggml_backend_dev_memory(m_dev, &freeMem, &totalMem);

    JOB_LOG_DEBUG(
        "[JobGgmlDevice] Registered: '{}' | Type: {} | Free: {:.1f} MB | Total: {:.1f} MB",
        m_name,
        static_cast<int>(m_props.type),
        static_cast<float>(freeMem) / (1024.0f * 1024.0f),
        static_cast<float>(totalMem) / (1024.0f * 1024.0f)
    );
}

JobGgmlDevice::~JobGgmlDevice()
{
    // ggml_backend_dev_t is owned by ggml's registry — we only free what we initialized
    if (m_backend) {
        ggml_backend_free(m_backend);
        m_backend = nullptr;
    }
}

const std::string &JobGgmlDevice::name() const noexcept
{
    return m_name;
}

enum ggml_backend_dev_type JobGgmlDevice::type() const noexcept
{
    return m_props.type;
}

size_t JobGgmlDevice::memoryFree() const noexcept
{
    size_t freeMem = 0;
    size_t totalMem = 0;

    // NOTE: This may be expensive depending on backend (e.g., CUDA)
    ggml_backend_dev_memory(m_dev, &freeMem, &totalMem);

    return freeMem;
}

size_t JobGgmlDevice::memoryTotal() const noexcept
{
    return m_props.memory_total;
}

const ggml_backend_dev_props &JobGgmlDevice::props() const noexcept
{
    return m_props;
}

ggml_backend_t JobGgmlDevice::backend() const noexcept
{
    return m_backend;
}

ggml_backend_buffer_type_t JobGgmlDevice::bufferType() const noexcept
{
    return m_bufferType;
}

ggml_backend_buffer_type_t JobGgmlDevice::hostBufferType() const noexcept
{
    return m_hostBufferType;
}

bool JobGgmlDevice::hasEvents() const noexcept
{
    return m_props.caps.events;
}

} // namespace job::ggml