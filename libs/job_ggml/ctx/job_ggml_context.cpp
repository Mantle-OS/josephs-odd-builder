#include "job_ggml_context.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlContext::JobGgmlContext(const JobGgmlInitParams &initParams) :
    JobGgmlContext{ggml_init_params{initParams.memorySize(), initParams.memoryBuffer(), initParams.noAlloc()}}
{
}

JobGgmlContext::JobGgmlContext(const ggml_init_params &initParams) :
    m_context{ggml_init(initParams)}
{
    if (!m_context) {
        throw std::runtime_error{
            "Failed to initialize GGML context"
        };
    }
}

JobGgmlContext::JobGgmlContext(ggml_context_ptr context) :
    m_context{std::move(context)}
{
    if (!m_context) {
        throw std::invalid_argument{
            "JobGgmlContext requires a valid ggml_context_ptr"
        };
    }
}

bool JobGgmlContext::isValid() const noexcept
{
    return m_context != nullptr;
}

ggml_context *JobGgmlContext::context() noexcept
{
    return m_context.get();
}

const ggml_context *JobGgmlContext::context() const noexcept
{
    return m_context.get();
}

void JobGgmlContext::reset() noexcept
{
    if (m_context)
        ggml_reset(m_context.get());
}

std::size_t JobGgmlContext::usedMemory() const noexcept
{
    return m_context ? ggml_used_mem(m_context.get()) : 0;
}

std::size_t JobGgmlContext::memorySize() const noexcept
{
    return m_context ? ggml_get_mem_size(m_context.get()) : 0;
}

std::size_t JobGgmlContext::maxTensorSize() const noexcept
{
    return m_context ? ggml_get_max_tensor_size(m_context.get()) : 0;
}

void *JobGgmlContext::memoryBuffer() noexcept
{
    return m_context ? ggml_get_mem_buffer(m_context.get()): nullptr;
}

const void *JobGgmlContext::memoryBuffer() const noexcept
{
    return m_context ? ggml_get_mem_buffer(m_context.get()) : nullptr;
}

bool JobGgmlContext::noAlloc() const noexcept
{
    return m_context && ggml_get_no_alloc(m_context.get());
}

void JobGgmlContext::setNoAlloc(bool noAlloc) noexcept
{
    if (!m_context)
        return;

    if (ggml_get_no_alloc(m_context.get()) != noAlloc)
        ggml_set_no_alloc(m_context.get(),noAlloc);
}

void *JobGgmlContext::newBuffer(std::size_t size)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot allocate memory from an invalid GGML context"
        };
    }

    if (size == 0) {
        throw std::invalid_argument{
            "newBuffer requires a size greater than zero"
        };
    }

    void *buffer = ggml_new_buffer(m_context.get(), size);

    if (!buffer) {
        throw std::runtime_error{
            "Failed to allocate a buffer from the GGML context"
        };
    }

    return buffer;
}

JobGgmlTensor::UPtr JobGgmlContext::newTensor(JobGgmlType type, int dimensions, const std::int64_t *extents)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor from an invalid GGML context"
        };
    }

    if (dimensions < 1 || dimensions > GGML_MAX_DIMS) {
        throw std::invalid_argument{
            "newTensor requires between one and four dimensions"
        };
    }

    if (!extents) {
        throw std::invalid_argument{
            "newTensor requires valid tensor extents"
        };
    }

    for (int i = 0; i < dimensions; ++i) {
        if (extents[i] <= 0) {
            throw std::invalid_argument{
                "Tensor extents must be greater than zero"
            };
        }
    }

    ggml_tensor *nativeTensor = ggml_new_tensor(
        m_context.get(),
        toGgmlType(type),
        dimensions,
        extents
        );

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create a GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::newTensor1d(JobGgmlType type, std::int64_t ne0)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor from an invalid GGML context"
        };
    }

    if (ne0 <= 0) {
        throw std::invalid_argument{
            "newTensor1d requires a positive extent"
        };
    }

    ggml_tensor *nativeTensor = ggml_new_tensor_1d(
        m_context.get(),
        toGgmlType(type),
        ne0
        );

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create a one-dimensional GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::newTensor2d(JobGgmlType type, std::int64_t ne0, std::int64_t ne1)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor from an invalid GGML context"
        };
    }

    if (ne0 <= 0 || ne1 <= 0) {
        throw std::invalid_argument{
            "newTensor2d requires positive extents"
        };
    }

    ggml_tensor *nativeTensor = ggml_new_tensor_2d(
        m_context.get(),
        toGgmlType(type),
        ne0, ne1
    );

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create a two-dimensional GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::newTensor3d(JobGgmlType type, std::int64_t ne0, std::int64_t ne1, std::int64_t ne2)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor from an invalid GGML context"
        };
    }

    if (ne0 <= 0 || ne1 <= 0 || ne2 <= 0) {
        throw std::invalid_argument{
            "newTensor3d requires positive extents"
        };
    }

    ggml_tensor *nativeTensor = ggml_new_tensor_3d(
        m_context.get(),
        toGgmlType(type),
        ne0,
        ne1,
        ne2
        );

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create a three-dimensional GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::newTensor4d(
    JobGgmlType type,
    std::int64_t ne0,
    std::int64_t ne1,
    std::int64_t ne2,
    std::int64_t ne3
    )
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor from an invalid GGML context"
        };
    }

    if (ne0 <= 0 || ne1 <= 0 || ne2 <= 0 || ne3 <= 0) {
        throw std::invalid_argument{
            "newTensor4d requires positive extents"
        };
    }

    ggml_tensor *nativeTensor = ggml_new_tensor_4d(
        m_context.get(),
        toGgmlType(type),
        ne0,
        ne1,
        ne2,
        ne3
        );

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create a four-dimensional GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::duplicateTensor(const JobGgmlTensor &source)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot duplicate a tensor with an invalid GGML context"
        };
    }

    if (!source.isValid()) {
        throw std::invalid_argument{
            "duplicateTensor requires a valid JobGgmlTensor"
        };
    }

    ggml_tensor *nativeTensor = ggml_dup_tensor(m_context.get(), source.tensor());

    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to duplicate the GGML tensor"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::viewTensor(JobGgmlTensor &source)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a tensor view with an invalid GGML context"
        };
    }

    if (!source.isValid()) {
        throw std::invalid_argument{
            "viewTensor requires a valid JobGgmlTensor"
        };
    }

    ggml_tensor *nativeTensor = ggml_view_tensor(m_context.get(), source.tensor());
    if (!nativeTensor) {
        throw std::runtime_error{
            "Failed to create the GGML tensor view"
        };
    }

    return JobGgmlTensor::createUniq(nativeTensor);
}

JobGgmlTensor::UPtr JobGgmlContext::firstTensor()
{
    if (!m_context)
        return nullptr;

    ggml_tensor *nativeTensor = ggml_get_first_tensor(m_context.get());

    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlContext::nextTensor(const JobGgmlTensor &tensor)
{
    if (!m_context ||
        !tensor.isValid()) {
        return nullptr;
    }

    ggml_tensor *nativeTensor = ggml_get_next_tensor(
        m_context.get(),
        const_cast<ggml_tensor *>( tensor.tensor() )
    );
    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlContext::tensor(const std::string &name)
{
    if (!m_context ||
        name.empty()) {
        return nullptr;
    }

    ggml_tensor *nativeTensor = ggml_get_tensor(m_context.get(), name.c_str());

    return nativeTensor ? JobGgmlTensor::createUniq(nativeTensor) : nullptr;
}

JobGgmlCGraph::UPtr JobGgmlContext::newGraph()
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a graph from an invalid GGML context"
        };
    }

    ggml_cgraph *nativeGraph = ggml_new_graph(m_context.get());

    if (!nativeGraph) {
        throw std::runtime_error{
            "Failed to create a GGML computation graph"
        };
    }

    return JobGgmlCGraph::createUniq(nativeGraph);
}

JobGgmlCGraph::UPtr JobGgmlContext::newGraphCustom(std::size_t size, bool gradients)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot create a graph from an invalid GGML context"
        };
    }

    if (size == 0) {
        throw std::invalid_argument{
            "newGraphCustom requires a graph size greater than zero"
        };
    }

    ggml_cgraph *nativeGraph = ggml_new_graph_custom(
        m_context.get(),
        size,
        gradients
        );

    if (!nativeGraph) {
        throw std::runtime_error{
            "Failed to create a custom GGML computation graph"
        };
    }

    return JobGgmlCGraph::createUniq(nativeGraph);
}

JobGgmlCGraph::UPtr JobGgmlContext::duplicateGraph(JobGgmlCGraph &graph, bool forceGradients)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot duplicate a graph with an invalid GGML context"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "duplicateGraph requires a valid JobGgmlCGraph"
        };
    }

    ggml_cgraph *nativeGraph = ggml_graph_dup(
        m_context.get(),
        graph.graph(),
        forceGradients
        );

    if (!nativeGraph) {
        throw std::runtime_error{
            "Failed to duplicate the GGML computation graph"
        };
    }

    return JobGgmlCGraph::createUniq( nativeGraph );
}

} // namespace job::ggml