#include "job_ggml_backend_graph_copy.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlBackendGraphCopy::JobGgmlBackendGraphCopy(JobGgmlBackend &backend, JobGgmlCGraph &graph)
{
    if (!backend.isValid())
        throw std::invalid_argument{ "JobGgmlBackendGraphCopy requires a valid JobGgmlBackend" };

    if (!graph.isValid())
        throw std::invalid_argument{ "JobGgmlBackendGraphCopy requires a valid JobGgmlCGraph" };

    m_graphCopy = ggml_backend_graph_copy(backend.backend(), graph.graph());

    if (!isValid()) {
        reset();
        throw std::runtime_error{ "Failed to copy the GGML graph to the requested backend" };
    }

}

JobGgmlBackendGraphCopy::~JobGgmlBackendGraphCopy()
{
    reset();
}

bool JobGgmlBackendGraphCopy::isValid() const noexcept
{
    return m_graphCopy.buffer          != nullptr &&
           m_graphCopy.ctx_allocated   != nullptr &&
           m_graphCopy.ctx_unallocated != nullptr &&
           m_graphCopy.graph           != nullptr;
}

JobGgmlBackendBufferView::UPtr JobGgmlBackendGraphCopy::bufferObject() const
{
    if (!m_graphCopy.buffer)
        return nullptr;

    return JobGgmlBackendBufferView::createUniq(m_graphCopy.buffer);
}

JobGgmlContextView::UPtr JobGgmlBackendGraphCopy::allocatedContextObject() const
{
    if (!m_graphCopy.ctx_allocated)
        return nullptr;

    return JobGgmlContextView::createUniq(m_graphCopy.ctx_allocated);
}

JobGgmlContextView::UPtr JobGgmlBackendGraphCopy::unallocatedContextObject() const
{
    if (!m_graphCopy.ctx_unallocated)
        return nullptr;

    return JobGgmlContextView::createUniq(m_graphCopy.ctx_unallocated);
}

JobGgmlCGraph::UPtr JobGgmlBackendGraphCopy::graphObject() const
{
    if (!m_graphCopy.graph)
        return nullptr;

    return JobGgmlCGraph::createUniq(m_graphCopy.graph);
}

ggml_backend_buffer_t JobGgmlBackendGraphCopy::buffer() const noexcept
{
    return m_graphCopy.buffer;
}

ggml_context *JobGgmlBackendGraphCopy::allocatedContext() noexcept
{
    return m_graphCopy.ctx_allocated;
}

const ggml_context *JobGgmlBackendGraphCopy::allocatedContext() const noexcept
{
    return m_graphCopy.ctx_allocated;
}

ggml_context *JobGgmlBackendGraphCopy::unallocatedContext() noexcept
{
    return m_graphCopy.ctx_unallocated;
}

const ggml_context *JobGgmlBackendGraphCopy::unallocatedContext() const noexcept
{
    return m_graphCopy.ctx_unallocated;
}

ggml_cgraph *JobGgmlBackendGraphCopy::graph() noexcept
{
    return m_graphCopy.graph;
}

const ggml_cgraph *JobGgmlBackendGraphCopy::graph() const noexcept
{
    return m_graphCopy.graph;
}

const struct ggml_backend_graph_copy &JobGgmlBackendGraphCopy::graphCopy() const noexcept
{
    return m_graphCopy;
}

void JobGgmlBackendGraphCopy::reset() noexcept
{
    if (m_graphCopy.buffer          != nullptr ||
        m_graphCopy.ctx_allocated   != nullptr ||
        m_graphCopy.ctx_unallocated != nullptr ||
        m_graphCopy.graph           != nullptr) {
        ggml_backend_graph_copy_free(m_graphCopy);
    }

    m_graphCopy = {};
}

} // namespace job::ggml