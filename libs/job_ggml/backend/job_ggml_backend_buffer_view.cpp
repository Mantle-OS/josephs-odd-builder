#include "job_ggml_backend_buffer_view.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlBackendBufferView::JobGgmlBackendBufferView(ggml_backend_buffer_t buffer) :
    m_buffer{buffer}
{
    if (!m_buffer)
        throw std::invalid_argument{ "JobGgmlBackendBufferView requires a valid ggml_backend_buffer_t" };

    const char *bufferName = ggml_backend_buffer_name(m_buffer);
    setName(bufferName ? bufferName : "unknown");
}

bool JobGgmlBackendBufferView::isValid() const noexcept
{
    return m_buffer != nullptr;
}

const std::string &JobGgmlBackendBufferView::name() const noexcept
{
    return m_name;
}

void JobGgmlBackendBufferView::setName(const std::string &name)
{
    if (m_name != name && !name.empty())
        m_name = name;
}

ggml_backend_buffer_t JobGgmlBackendBufferView::buffer() const noexcept
{
    return m_buffer;
}

ggml_backend_buffer_type_t JobGgmlBackendBufferView::bufferType() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_type(m_buffer) : nullptr;
}

bool JobGgmlBackendBufferView::isHost() const noexcept
{
    return m_buffer && ggml_backend_buffer_is_host(m_buffer);
}

void *JobGgmlBackendBufferView::base() noexcept
{
    return m_buffer ? ggml_backend_buffer_get_base(m_buffer) : nullptr;
}

const void *JobGgmlBackendBufferView::base() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_base(m_buffer) : nullptr;
}

std::size_t JobGgmlBackendBufferView::size() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_size(m_buffer) : 0;
}

std::size_t JobGgmlBackendBufferView::alignment() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_alignment(m_buffer) : 0;
}

std::size_t JobGgmlBackendBufferView::maxSize() const noexcept
{
    return m_buffer ? ggml_backend_buffer_get_max_size(m_buffer) : 0;
}

std::size_t JobGgmlBackendBufferView::allocationSize(const JobGgmlTensor &tensor) const noexcept
{
    if (!tensor.isValid())
        return 0;

    return allocationSize(tensor.tensor());
}

std::size_t JobGgmlBackendBufferView::allocationSize(const ggml_tensor *tensor) const noexcept
{
    if (!m_buffer || !tensor)
        return 0;

    return ggml_backend_buffer_get_alloc_size(m_buffer, tensor);
}

JobGgmlBackendBufferUsage JobGgmlBackendBufferView::usage() const noexcept
{
    if (!m_buffer)
        return JobGgmlBackendBufferUsage::Any;

    return static_cast<JobGgmlBackendBufferUsage>(ggml_backend_buffer_get_usage(m_buffer));
}

} // namespace job::ggml