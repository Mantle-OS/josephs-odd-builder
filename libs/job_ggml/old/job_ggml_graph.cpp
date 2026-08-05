    #include "job_ggml_graph.h"

#include <job_logger.h>
#include <stdexcept>

namespace job::ggml {

JobGgmlGraph::JobGgmlGraph(size_t memBytes, size_t graphSize)
    : m_memBytes(memBytes)
{
    struct ggml_init_params params{};
    params.mem_size   = memBytes;
    params.mem_buffer = nullptr;
    // params.no_alloc   = false;
    params.no_alloc = true;

    m_ctx = ggml_context_ptr{ggml_init(params)};
    if (!m_ctx) {
        JOB_LOG_ERROR("[JobGgmlGraph] Failed to allocate {} bytes for ggml context", memBytes);
        throw std::runtime_error("JobGgmlGraph: out of memory for context");
    }

    m_graph = ggml_new_graph_custom(m_ctx.get(), graphSize, true);
    if (!m_graph) {
        JOB_LOG_ERROR("[JobGgmlGraph] Failed to create graph with size {}", graphSize);
        throw std::runtime_error("JobGgmlGraph: failed to create graph");
    }
}

ggml_context *JobGgmlGraph::context() noexcept
{
    return m_ctx.get();
}

ggml_cgraph *JobGgmlGraph::graph() noexcept
{
    return m_graph;
}

ggml_tensor *JobGgmlGraph::tensor1d(int64_t ne0, const std::string &name)
{
    auto *t = ggml_new_tensor_1d(m_ctx.get(), GGML_TYPE_F32, ne0);
    if (!t)
        throw std::runtime_error("tensor1d allocation failed");

    if (!name.empty())
        ggml_set_name(t, name.c_str());

    return t;
}

ggml_tensor *JobGgmlGraph::tensor2d(int64_t ne0, int64_t ne1, const std::string &name)
{
    auto *t = ggml_new_tensor_2d(m_ctx.get(), GGML_TYPE_F32, ne0, ne1);
    if (!t)
        throw std::runtime_error("tensor2d allocation failed");

    if (!name.empty())
        ggml_set_name(t, name.c_str());

    return t;
}

ggml_tensor *JobGgmlGraph::tensor3d(int64_t ne0, int64_t ne1, int64_t ne2, const std::string &name)
{
    auto *t = ggml_new_tensor_3d(m_ctx.get(), GGML_TYPE_F32, ne0, ne1, ne2);
    if (!t)
        throw std::runtime_error("tensor3d allocation failed");

    if (!name.empty())
        ggml_set_name(t, name.c_str());

    return t;
}

ggml_tensor *JobGgmlGraph::tensor4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3, const std::string &name)
{
    auto *t = ggml_new_tensor_4d(m_ctx.get(), GGML_TYPE_F32, ne0, ne1, ne2, ne3);
    if (!t)
        throw std::runtime_error("tensor4d allocation failed");

    if (!name.empty())
        ggml_set_name(t, name.c_str());

    return t;
}


void JobGgmlGraph::markParam(ggml_tensor *t)
{
    ggml_set_param(t);
}

void JobGgmlGraph::addForward(ggml_tensor *result)
{
    ggml_build_forward_expand(m_graph, result);
}

void JobGgmlGraph::addBackward(ggml_tensor **gradAccs)
{
    int nNodes = ggml_graph_n_nodes(m_graph);

    if (nNodes <= 0) {
        JOB_LOG_WARN("[JobGgmlGraph] Cannot build backward — graph is empty");
        return;
    }

    // safer than relying on -1 magic
    ggml_tensor *loss = ggml_graph_node(m_graph, nNodes - 1);
    ggml_set_loss(loss);

    ggml_build_backward_expand(m_ctx.get(), m_graph, gradAccs);
}

void JobGgmlGraph::compute(JobGgmlDevice &device)
{
    auto backend = device.backend();
    if (!backend) {
        JOB_LOG_ERROR("[JobGgmlGraph] Device '{}' has no backend", device.name());
        return;
    }

    auto status = ggml_backend_graph_compute(backend, m_graph);

    if (status != GGML_STATUS_SUCCESS) {
        JOB_LOG_ERROR("[JobGgmlGraph] Graph compute failed on '{}' with status {}",
                      device.name(),
                      static_cast<int>(status));
    }

    // ensure completion (important for async backends)
    ggml_backend_synchronize(backend);
}

void JobGgmlGraph::computeWithSched(JobGgmlDeviceManager &manager) {
    auto* sched = manager.scheduler();
    if (!sched)
        return;

    // IMPORTANT: If the graph is larger than the scheduler's initial capacity,
    // it needs to be reserved/re-allocated.
    if (ggml_backend_sched_reserve(sched, m_graph) != GGML_STATUS_SUCCESS) {
        JOB_LOG_ERROR("Failed to reserve scheduler memory for graph");
        return;
    }

    if (ggml_backend_sched_graph_compute(sched, m_graph) != GGML_STATUS_SUCCESS) {
        JOB_LOG_ERROR("Graph execution failed");
        return;
    }
}
void JobGgmlGraph::reset()
{
    if (!m_ctx)
        return;
    ggml_graph_clear(m_graph);

    // m_graph = ggml_new_graph_custom(m_ctx.get(), kDefaultGraphSize, true);
}

size_t JobGgmlGraph::usedMem() const noexcept
{
    return ggml_used_mem(m_ctx.get());
}

} // namespace job::ggml