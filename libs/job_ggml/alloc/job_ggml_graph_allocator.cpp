#include "job_ggml_graph_allocator.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace job::ggml {

JobGgmlGraphAllocator::JobGgmlGraphAllocator(JobGgmlBackendBufferType::Ptr bufferType)
{
    std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes;
    bufferTypes.push_back(std::move(bufferType));

    initialize(std::move(bufferTypes));
}

JobGgmlGraphAllocator::JobGgmlGraphAllocator(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes)
{
    initialize(std::move(bufferTypes));
}

JobGgmlGraphAllocator::~JobGgmlGraphAllocator()
{
    clear();
}

bool JobGgmlGraphAllocator::isValid() const noexcept
{
    return m_graphAllocator != nullptr && !m_bufferTypes.empty();
}

ggml_gallocr_t JobGgmlGraphAllocator::graphAllocator() const noexcept
{
    return m_graphAllocator;
}

std::size_t JobGgmlGraphAllocator::bufferCount() const noexcept
{
    return m_bufferTypes.size();
}

JobGgmlBackendBufferType::Ptr JobGgmlGraphAllocator::bufferType(std::size_t index) const noexcept
{
    if (index >= m_bufferTypes.size())
        return nullptr;

    return m_bufferTypes[index];
}

bool JobGgmlGraphAllocator::reserve(
    JobGgmlCGraph &graph
    )
{
    if (!isValid())
        throw std::runtime_error{
            "Cannot reserve a graph with an invalid GGML graph allocator"
        };

    if (!graph.isValid())
        throw std::invalid_argument{
            "reserve requires a valid JobGgmlCGraph"
        };

    return ggml_gallocr_reserve(m_graphAllocator, graph.graph());
}

void JobGgmlGraphAllocator::reserveSize(JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds, std::vector<std::size_t> &sizes)
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot measure graph allocation with an invalid GGML graph allocator"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "reserveSize requires a valid JobGgmlCGraph"
        };
    }

    if (!validateAssignments(graph, nodeBufferIds, leafBufferIds)) {
        throw std::invalid_argument{
            "reserveSize received invalid node or leaf buffer assignments"
        };
    }

    sizes.assign(bufferCount(), std::size_t{0});

    ggml_gallocr_reserve_n_size(
        m_graphAllocator,
        graph.graph(),
        nodeBufferIds.data(),
        leafBufferIds.data(),
        sizes.data()
        );
}

bool JobGgmlGraphAllocator::reserve(JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds)
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot reserve a graph with an invalid GGML graph allocator"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "reserve requires a valid JobGgmlCGraph"
        };
    }

    if (!validateAssignments(graph, nodeBufferIds, leafBufferIds)) {
        throw std::invalid_argument{
            "reserve received invalid node or leaf buffer assignments"
        };
    }

    return ggml_gallocr_reserve_n(m_graphAllocator, graph.graph(), nodeBufferIds.data(), leafBufferIds.data());
}

bool JobGgmlGraphAllocator::allocateGraph(JobGgmlCGraph &graph)
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot allocate a graph with an invalid GGML graph allocator"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "allocateGraph requires a valid JobGgmlCGraph"
        };
    }

    return ggml_gallocr_alloc_graph(m_graphAllocator, graph.graph());
}

std::size_t JobGgmlGraphAllocator::bufferSize(int bufferId) const noexcept
{
    if (!isValid() || bufferId < 0 || static_cast<std::size_t>(bufferId) >= bufferCount())
        return 0;

    return ggml_gallocr_get_buffer_size(m_graphAllocator, bufferId);
}

void JobGgmlGraphAllocator::reset(JobGgmlBackendBufferType::Ptr bufferType)
{
    std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes;
    bufferTypes.push_back(std::move(bufferType));

    reset(std::move(bufferTypes));
}

void JobGgmlGraphAllocator::reset(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes)
{
    clear();
    initialize(std::move(bufferTypes));
}

void JobGgmlGraphAllocator::clear() noexcept
{
    if (m_graphAllocator) {
        ggml_gallocr_free(m_graphAllocator);
        m_graphAllocator = nullptr;
    }

    m_bufferTypes.clear();
}

void JobGgmlGraphAllocator::initialize(std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes)
{
    if (bufferTypes.empty()) {
        throw std::invalid_argument{
            "JobGgmlGraphAllocator requires at least one buffer type"
        };
    }

    std::vector<ggml_backend_buffer_type_t> nativeBufferTypes;
    nativeBufferTypes.reserve(bufferTypes.size());

    for (const auto &bufferType : bufferTypes) {
        if (!bufferType || !bufferType->isValid()) {
            throw std::invalid_argument{
                "JobGgmlGraphAllocator received an invalid buffer type"
            };
        }

        nativeBufferTypes.push_back(bufferType->bufferType());
    }

    ggml_gallocr_t graphAllocator = nullptr;

    if (nativeBufferTypes.size() == 1)
        graphAllocator = ggml_gallocr_new(nativeBufferTypes.front());
    else
        graphAllocator = ggml_gallocr_new_n(nativeBufferTypes.data(), static_cast<int>(nativeBufferTypes.size()));

    if (!graphAllocator) {
        throw std::runtime_error{
            "Failed to create the GGML graph allocator"
        };
    }

    /*
     * Publish the retained wrappers only after native construction succeeds.
     * This prevents a partially initialized allocator object.
     */
    m_bufferTypes    = std::move(bufferTypes);
    m_graphAllocator = graphAllocator;
}

bool JobGgmlGraphAllocator::validateAssignments(const JobGgmlCGraph &graph, const std::vector<int> &nodeBufferIds, const std::vector<int> &leafBufferIds) const noexcept
{
    if (!isValid() || !graph.isValid())
        return false;

    if (nodeBufferIds.size() !=
        static_cast<std::size_t>(graph.nodeCount())) {
        return false;
    }

    /*
     * GGML does not expose the graph leaf count through its public API.
     * so ... cannot validate leafBufferIds.size() here without depending on private ggml_cgraph internals.
     */
    const auto validBufferId = [this](int bufferId) noexcept {
        return bufferId >= 0 && static_cast<std::size_t>(bufferId) < bufferCount();
    };

    for (const int bufferId : nodeBufferIds)
        if (!validBufferId(bufferId))
            return false;

    for (const int bufferId : leafBufferIds)
        if (!validBufferId(bufferId))
            return false;

    return true;
}

} // namespace job::ggml