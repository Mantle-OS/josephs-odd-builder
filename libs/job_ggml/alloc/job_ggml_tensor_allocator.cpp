#include "job_ggml_tensor_allocator.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlTensorAllocator::JobGgmlTensorAllocator(JobGgmlBackendBuffer::Ptr buffer) :
    m_buffer{std::move(buffer)}
{
    if (!m_buffer || !m_buffer->isValid()) {
        throw std::invalid_argument{
            "JobGgmlTensorAllocator requires a valid JobGgmlBackendBuffer"
        };
    }

    resetTensorAllocator();
}

JobGgmlTensorAllocator::JobGgmlTensorAllocator(JobGgmlBackendBuffer::Ptr buffer, const struct ggml_tallocr &tensorAllocator) :
    m_buffer{std::move(buffer)}
{
    if (!m_buffer || !m_buffer->isValid()) {
        throw std::invalid_argument{
            "JobGgmlTensorAllocator requires a valid JobGgmlBackendBuffer"
        };
    }

    if (!tensorAllocator.buffer) {
        throw std::invalid_argument{
            "JobGgmlTensorAllocator requires a valid ggml_tallocr buffer"
        };
    }

    if (tensorAllocator.buffer != m_buffer->buffer()) {
        throw std::invalid_argument{
            "The ggml_tallocr buffer does not match the supplied JobGgmlBackendBuffer"
        };
    }

    setTensorAllocator(tensorAllocator);
}

bool JobGgmlTensorAllocator::isValid() const noexcept
{
    return m_buffer &&
           m_buffer->isValid() &&
           m_tensorAllocator.buffer == m_buffer->buffer() &&
           m_base != nullptr &&
           m_alignment > 0;
}

JobGgmlBackendBuffer::Ptr JobGgmlTensorAllocator::buffer() const noexcept
{
    return m_buffer;
}

void *JobGgmlTensorAllocator::base() noexcept
{
    return m_base;
}

const void *JobGgmlTensorAllocator::base() const noexcept
{
    return m_base;
}

std::size_t JobGgmlTensorAllocator::alignment() const noexcept
{
    return m_alignment;
}

std::size_t JobGgmlTensorAllocator::offset() const noexcept
{
    return m_offset;
}

std::size_t JobGgmlTensorAllocator::capacity() const noexcept
{
    return m_buffer ? m_buffer->size() : 0;
}

std::size_t JobGgmlTensorAllocator::remaining() const noexcept
{
    const std::size_t bufferCapacity = capacity();

    return m_offset <= bufferCapacity ? bufferCapacity - m_offset : 0;
}

JobGgmlStatus JobGgmlTensorAllocator::allocate(JobGgmlTensor &tensor) noexcept
{
    if (!tensor.isValid())
        return JobGgmlStatus::Failed;

    return allocate(tensor.tensor());
}

JobGgmlStatus JobGgmlTensorAllocator::allocate(struct ggml_tensor *tensor) noexcept
{
    if (!isValid() || !tensor)
        return JobGgmlStatus::Failed;

    const enum ggml_status status = ggml_tallocr_alloc(&m_tensorAllocator, tensor);

    /*
     * ggml_tallocr_alloc() advances the native allocator offset.
     * Synchronize the C++ proxy members from the updated value.
     */
    setTensorAllocator(m_tensorAllocator);

    return fromGgmlStatus(status);
}

void JobGgmlTensorAllocator::setTensorAllocator(const struct ggml_tallocr &other) noexcept
{
    if (!m_buffer || !m_buffer->isValid())
        return;

    /*
     * This wrapper retains the JobGgmlBackendBuffer that owns the native
     * buffer. Never allow the allocator proxy to switch to an unrelated
     * borrowed buffer handle.
     */
    if (other.buffer != m_buffer->buffer())
        return;

    m_base      = other.base;
    m_alignment = other.alignment;
    m_offset    = other.offset;

    m_tensorAllocator = other;
}

struct ggml_tallocr
JobGgmlTensorAllocator::tensorAllocator() noexcept
{
    struct ggml_tallocr ret{
        m_buffer ? m_buffer->buffer() : nullptr,
        m_base,
        m_alignment,
        m_offset
    };

    m_tensorAllocator = ret;

    return m_tensorAllocator;
}

void JobGgmlTensorAllocator::resetTensorAllocator() noexcept
{
    if (!m_buffer || !m_buffer->isValid()) {
        m_tensorAllocator = {};
        m_base             = nullptr;
        m_alignment        = 0;
        m_offset           = 0;
        return;
    }

    m_tensorAllocator = ggml_tallocr_new(
        m_buffer->buffer()
        );

    setTensorAllocator(m_tensorAllocator);
}

} // namespace job::ggml