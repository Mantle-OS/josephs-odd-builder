#include "job_ggml_backend_buffer_type.h"

#include <stdexcept>


namespace job::ggml {

JobGgmlBackendBufferType::JobGgmlBackendBufferType(ggml_backend_buffer_type_t bufferType) :
    m_bufferType{bufferType}
{
    if (!m_bufferType)
        throw std::invalid_argument{ "JobGgmlBackendBufferType requires a valid ggml_backend_buffer_type_t" };

    const char *nativeName = ggml_backend_buft_name(m_bufferType);
    setName(nativeName ? nativeName : "unknown");
    setAlignment(ggml_backend_buft_get_alignment(m_bufferType));
    setMaxSize(ggml_backend_buft_get_max_size(m_bufferType));
    setIsHost(ggml_backend_buft_is_host(m_bufferType));

    m_device = ggml_backend_buft_get_device(m_bufferType);
}

const std::string &JobGgmlBackendBufferType::name() const noexcept
{
    return m_name;
}

void JobGgmlBackendBufferType::setName(const std::string &name)
{
    if (m_name != name && !name.empty())
        m_name = name;
}

std::size_t JobGgmlBackendBufferType::alignment() const noexcept
{
    return m_alignment;
}

void JobGgmlBackendBufferType::setAlignment(std::size_t alignment) noexcept
{
    if (m_alignment != alignment)
        m_alignment = alignment;
}

std::size_t JobGgmlBackendBufferType::maxSize() const noexcept
{
    return m_maxSize;
}

void JobGgmlBackendBufferType::setMaxSize(std::size_t maxSize) noexcept
{
    if (m_maxSize != maxSize)
        m_maxSize = maxSize;
}

bool JobGgmlBackendBufferType::isHost() const noexcept
{
    return m_isHost;
}

void JobGgmlBackendBufferType::setIsHost(bool isHost) noexcept
{
    if (m_isHost != isHost)
        m_isHost = isHost;
}

std::unique_ptr<JobGgmlBackendBuffer> JobGgmlBackendBufferType::allocateBuffer(std::size_t size) const
{
    if (!isValid())
        throw std::runtime_error{ "Cannot allocate a buffer from an invalid GGML backend buffer type" };

    if (size == 0)
        throw std::invalid_argument{ "allocateBuffer requires a size greater than zero" };

    if (m_maxSize > 0 && size > m_maxSize)
        throw std::length_error{ "Requested GGML backend buffer exceeds the maximum supported size" };

    ggml_backend_buffer_ptr nativeBuffer{
        ggml_backend_buft_alloc_buffer(m_bufferType, size)
    };

    if (!nativeBuffer)
        throw std::runtime_error{ "Failed to allocate a GGML backend buffer" };

    return std::make_unique<JobGgmlBackendBuffer>(std::move(nativeBuffer));
}

std::size_t JobGgmlBackendBufferType::allocationSize(const JobGgmlTensor &tensor) const noexcept
{
    if (!tensor.isValid())
        return 0;

    return allocationSize(tensor.tensor());
}

std::size_t JobGgmlBackendBufferType::allocationSize(const ggml_tensor *tensor) const noexcept
{
    if (!isValid() || !tensor)
        return 0;

    return ggml_backend_buft_get_alloc_size(m_bufferType, tensor);
}

ggml_backend_dev_t JobGgmlBackendBufferType::device() const noexcept
{
    return m_device;
}

ggml_backend_buffer_type_t JobGgmlBackendBufferType::bufferType() const noexcept
{
    return m_bufferType;
}

bool JobGgmlBackendBufferType::isValid() const noexcept
{
    return m_bufferType != nullptr;
}

} // namespace job::ggml