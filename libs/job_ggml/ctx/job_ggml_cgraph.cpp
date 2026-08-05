#include "job_ggml_cgraph.h"
#include "job_ggml_context.h"
#include "job_ggml_tensor.h"
#include <stdexcept>

namespace job::ggml {

JobGgmlCGraph::JobGgmlCGraph(ggml_cgraph *graph) :
    m_graph{graph}
{
    if (!m_graph)
        throw std::invalid_argument{"JobGgmlCGraph requires a valid ggml_cgraph pointer"};
}

bool JobGgmlCGraph::isValid() const noexcept
{
    return m_graph != nullptr;
}

int JobGgmlCGraph::size() const noexcept
{
    return m_graph ? ggml_graph_size(m_graph) : 0;
}

int JobGgmlCGraph::nodeCount() const noexcept
{
    return m_graph ? ggml_graph_n_nodes(m_graph) : 0;
}

JobGgmlTensor::UPtr JobGgmlCGraph::node(int index) const
{
    if (!m_graph)
        return nullptr;

    const int count = ggml_graph_n_nodes(m_graph);

    if (index >= count || index < -count)
        return nullptr;

    ggml_tensor *nativeTensor = ggml_graph_node(m_graph, index);
    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

std::vector<JobGgmlTensor::UPtr> JobGgmlCGraph::nodes() const
{
    std::vector<JobGgmlTensor::UPtr> ret;

    if (!m_graph)
        return ret;

    const int count = nodeCount();

    if (count <= 0)
        return ret;

    ret.reserve(
        static_cast<std::size_t>(count)
        );

    ggml_tensor **nativeNodes = ggml_graph_nodes(m_graph);

    if (!nativeNodes)
        return ret;

    for (int i = 0; i < count; ++i) {
        if (!nativeNodes[i])
            continue;

        ret.push_back(JobGgmlTensor::createUniq(nativeNodes[i]));
    }

    return ret;
}

JobGgmlTensor::UPtr JobGgmlCGraph::tensor(const std::string &name) const
{
    if (!m_graph || name.empty())
        return nullptr;

    ggml_tensor *nativeTensor = ggml_graph_get_tensor(m_graph, name.c_str());

    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlCGraph::gradient(const JobGgmlTensor &node) const
{
    if (!m_graph || !node.isValid())
        return nullptr;

    ggml_tensor *nativeGradient = ggml_graph_get_grad(m_graph, node.tensor());

    return nativeGradient ? JobGgmlTensor::createUniq(nativeGradient) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlCGraph::gradientAccumulator(const JobGgmlTensor &node) const
{
    if (!m_graph || !node.isValid())
        return nullptr;

    ggml_tensor *nativeAccumulator = ggml_graph_get_grad_acc(m_graph, node.tensor());

    return nativeAccumulator ? JobGgmlTensor::createUniq(nativeAccumulator) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlCGraph::buildForwardSelect(const std::vector<JobGgmlTensor *> &tensors, int index)
{
    if (!m_graph || tensors.empty())
        return nullptr;

    if (index < 0 ||
        static_cast<std::size_t>(index) >= tensors.size()) {
        return nullptr;
    }

    std::vector<ggml_tensor *> nativeTensors;
    nativeTensors.reserve(tensors.size());

    for (JobGgmlTensor *tensor : tensors) {
        if (!tensor || !tensor->isValid())
            return nullptr;

        nativeTensors.push_back(tensor->tensor());
    }

    ggml_tensor *selected = ggml_build_forward_select(
        m_graph,
        nativeTensors.data(),
        static_cast<int>(nativeTensors.size()),
        index
        );

    return selected ? JobGgmlTensor::createUniq(selected) : nullptr;
}

void JobGgmlCGraph::buildForwardExpand(JobGgmlTensor &tensor)
{
    if (!m_graph || !tensor.isValid())
        return;

    ggml_build_forward_expand(m_graph, tensor.tensor());
}


void JobGgmlCGraph::buildBackwardExpand(JobGgmlContext &context, const std::vector<JobGgmlTensor *> &gradientAccumulators)
{
    if (!m_graph || !context.isValid())
        return;

    if (gradientAccumulators.empty()) {
        ggml_build_backward_expand(context.context(), m_graph, nullptr);
        return;
    }

    std::vector<ggml_tensor *> nativeAccumulators;
    nativeAccumulators.reserve(gradientAccumulators.size());

    for (JobGgmlTensor *tensor : gradientAccumulators) {
        if (!tensor || !tensor->isValid())
            return;

        nativeAccumulators.push_back(tensor->tensor());
    }

    ggml_build_backward_expand(context.context(), m_graph, nativeAccumulators.data());
}

void JobGgmlCGraph::addNode(JobGgmlTensor &tensor)
{
    if (!m_graph || !tensor.isValid())
        return;

    ggml_graph_add_node(m_graph, tensor.tensor());
}

void JobGgmlCGraph::reset() noexcept
{
    if (m_graph)
        ggml_graph_reset(m_graph);
}

void JobGgmlCGraph::clear() noexcept
{
    if (m_graph)
        ggml_graph_clear(m_graph);
}

void JobGgmlCGraph::print() const noexcept
{
    if (m_graph)
        ggml_graph_print(m_graph);
}

void JobGgmlCGraph::dumpDot(const std::string &fileName, const JobGgmlCGraph *forwardGraph) const
{
    if (!m_graph || fileName.empty())
        return;

    ggml_graph_dump_dot(
        forwardGraph ? forwardGraph->graph() : nullptr,
        m_graph,
        fileName.c_str()
        );
}

ggml_cgraph *JobGgmlCGraph::graph() noexcept
{
    return m_graph;
}

const ggml_cgraph *JobGgmlCGraph::graph() const noexcept
{
    return m_graph;
}

std::size_t JobGgmlCGraph::overhead() noexcept
{
    return ggml_graph_overhead();
}

std::size_t JobGgmlCGraph::overheadCustom(std::size_t size, bool gradients) noexcept
{
    return ggml_graph_overhead_custom(size, gradients);
}

} // namespace job::ggml