#include "job_ggml_backend_buffer.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlBackendBuffer::JobGgmlBackendBuffer(ggml_backend_buffer_t buffer) :
    m_buffer{buffer}
{
    if (!m_buffer)
        throw std::invalid_argument{"JobGgmlBackendBuffer requires a valid ggml_backend_buffer_t"};

    const char *bufferName = ggml_backend_buffer_name(m_buffer.get());
    setName(bufferName ? bufferName : "unknown");
}

JobGgmlBackendBuffer::JobGgmlBackendBuffer(ggml_backend_buffer_ptr buffer) :
    m_buffer{std::move(buffer)}
{
    if (!m_buffer)
        throw std::invalid_argument{"JobGgmlBackendBuffer requires a valid ggml_backend_buffer_ptr"};

    const char *bufferName = ggml_backend_buffer_name(m_buffer.get());
    setName(bufferName ? bufferName : "unknown");
}

const std::string &JobGgmlBackendBuffer::name() const noexcept
{
    return m_name;
}

void JobGgmlBackendBuffer::setName(const std::string &name)
{
    if (m_name != name && !name.empty())
        m_name = name;
}

ggml_backend_buffer_t JobGgmlBackendBuffer::buffer() const noexcept
{
    return m_buffer.get();
}

ggml_backend_buffer_type_t JobGgmlBackendBuffer::bufferType() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_type(m_buffer.get()) : nullptr;
}

bool JobGgmlBackendBuffer::isValid() const noexcept
{
    return m_buffer != nullptr;
}

bool JobGgmlBackendBuffer::isHost() const noexcept
{
    return m_buffer && ggml_backend_buffer_is_host(m_buffer.get());
}

void *JobGgmlBackendBuffer::base() noexcept
{
    return m_buffer ? ggml_backend_buffer_get_base(m_buffer.get()) : nullptr;
}

const void *JobGgmlBackendBuffer::base() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_base(m_buffer.get()) : nullptr;
}

std::size_t JobGgmlBackendBuffer::size() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_size(m_buffer.get()) : 0;
}

std::size_t JobGgmlBackendBuffer::alignment() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_alignment(m_buffer.get()) : 0;
}

std::size_t JobGgmlBackendBuffer::maxSize() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_max_size(m_buffer.get()) : 0;
}

std::size_t JobGgmlBackendBuffer::allocationSize(const JobGgmlTensor &tensor) const noexcept
{
    if (!tensor.isValid())
        return 0;

    return allocationSize(tensor.tensor());
}

std::size_t JobGgmlBackendBuffer::allocationSize(const ggml_tensor *tensor) const noexcept
{
    if (!m_buffer || !tensor)
        return 0;

    return ggml_backend_buffer_get_alloc_size(m_buffer.get(), tensor);
}

JobGgmlStatus JobGgmlBackendBuffer::initializeTensor(JobGgmlTensor &tensor)
{
    if (!tensor.isValid())
        return JobGgmlStatus::Failed;

    return initializeTensor(tensor.tensor());
}

JobGgmlStatus JobGgmlBackendBuffer::initializeTensor(ggml_tensor *tensor)
{
    if (!m_buffer || !tensor)
        return JobGgmlStatus::Failed;

    return static_cast<JobGgmlStatus>(ggml_backend_buffer_init_tensor(m_buffer.get(), tensor));
}

JobGgmlBackendBufferUsage JobGgmlBackendBuffer::usage() const noexcept
{
    if (!m_buffer)
        return JobGgmlBackendBufferUsage::Any;

    return fromGgmlBufferUsage(ggml_backend_buffer_get_usage(m_buffer.get()));
}

void JobGgmlBackendBuffer::setUsage(JobGgmlBackendBufferUsage usage) noexcept
{
    if (!m_buffer)
        return;

    const enum ggml_backend_buffer_usage nativeUsage = toGgmlBufferUsage(usage);

    if (ggml_backend_buffer_get_usage(m_buffer.get()) != nativeUsage)
        ggml_backend_buffer_set_usage(m_buffer.get(), nativeUsage);
}

void JobGgmlBackendBuffer::clear(std::uint8_t value) noexcept
{
    if (m_buffer)
        ggml_backend_buffer_clear(m_buffer.get(), value);
}

void JobGgmlBackendBuffer::reset() noexcept
{
    if (m_buffer)
        ggml_backend_buffer_reset(m_buffer.get());
}

} // namespace job::ggml