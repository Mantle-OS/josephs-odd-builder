#include "job_ggml_backend_sched.h"

#include <stdexcept>
#include <utility>
#include <vector>

namespace job::ggml {
JobGgmlBackendSched::JobGgmlBackendSched(std::vector<JobGgmlBackend::Ptr> backends, std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes, std::size_t graphSize, bool parallel, bool opOffload) :
    m_backends{std::move(backends)},
    m_bufferTypes{std::move(bufferTypes)},
    m_graphSize{graphSize},
    m_parallel{parallel},
    m_opOffload{opOffload}
{
    if (m_backends.empty()) {
        throw std::invalid_argument{
            "JobGgmlBackendSched requires at least one backend"
        };
    }

    if (m_graphSize == 0) {
        throw std::invalid_argument{
            "JobGgmlBackendSched requires a graph size greater than zero"
        };
    }

    if (!m_bufferTypes.empty() &&
        m_bufferTypes.size() != m_backends.size()) {
        throw std::invalid_argument{
            "JobGgmlBackendSched requires one buffer type per backend"
        };
    }

    std::vector<ggml_backend_t> nativeBackends;
    nativeBackends.reserve(m_backends.size());

    for (const auto &backend : m_backends) {
        if (!backend || !backend->isValid()) {
            throw std::invalid_argument{
                "JobGgmlBackendSched received an invalid backend"
            };
        }

        nativeBackends.push_back(backend->backend());
    }

    std::vector<ggml_backend_buffer_type_t> nativeBufferTypes;

    if (!m_bufferTypes.empty()) {
        nativeBufferTypes.reserve(m_bufferTypes.size());

        for (const auto &bufferType : m_bufferTypes) {
            if (!bufferType || !bufferType->isValid()) {
                throw std::invalid_argument{
                    "JobGgmlBackendSched received an invalid buffer type"
                };
            }

            nativeBufferTypes.push_back(bufferType->bufferType());
        }
    }

    m_scheduler.reset(
        ggml_backend_sched_new(
            nativeBackends.data(),
            nativeBufferTypes.empty() ? nullptr : nativeBufferTypes.data(),
            static_cast<int>(nativeBackends.size()),
            m_graphSize,
            m_parallel,
            m_opOffload
            )
        );

    if (!m_scheduler) {
        throw std::runtime_error{
            "Failed to create GGML backend scheduler"
        };
    }

    // If the caller did not explicitly supply buffer types, GGML selected the defaults. Wrap them now so the rest of the scheduler always has canonical buffer-type objects.
    if (m_bufferTypes.empty()) {
        m_bufferTypes.reserve(m_backends.size());
        for (const auto &backend : m_backends) {
            ggml_backend_buffer_type_t nativeBufferType = ggml_backend_sched_get_buffer_type( m_scheduler.get(), backend->backend());
            if (!nativeBufferType) {
                throw std::runtime_error{
                    "GGML scheduler failed to provide a backend buffer type"
                };
            }

            m_bufferTypes.push_back(JobGgmlBackendBufferType::createShared(nativeBufferType));
        }
    }
}

ggml_backend_sched_t JobGgmlBackendSched::scheduler() const noexcept
{
    return m_scheduler.get();
}

bool JobGgmlBackendSched::isValid() const noexcept
{
    return m_scheduler != nullptr;
}

std::size_t JobGgmlBackendSched::graphSize() const noexcept
{
    return m_graphSize;
}

bool JobGgmlBackendSched::parallel() const noexcept
{
    return m_parallel;
}

bool JobGgmlBackendSched::opOffload() const noexcept
{
    return m_opOffload;
}

int JobGgmlBackendSched::backendCount() const noexcept
{
    if (!m_scheduler)
        return 0;

    return ggml_backend_sched_get_n_backends(m_scheduler.get());
}

JobGgmlBackend::Ptr JobGgmlBackendSched::backend(int index) const noexcept
{
    if (!m_scheduler || index < 0 || index >= backendCount())
        return nullptr;

    return backendFromNative( ggml_backend_sched_get_backend(m_scheduler.get(), index) );
}

int JobGgmlBackendSched::splitCount() const noexcept
{
    if (!m_scheduler)
        return 0;

    return ggml_backend_sched_get_n_splits(m_scheduler.get());
}

int JobGgmlBackendSched::copyCount() const noexcept
{
    if (!m_scheduler)
        return 0;

    return ggml_backend_sched_get_n_copies(m_scheduler.get());
}

JobGgmlBackendBufferType::Ptr JobGgmlBackendSched::bufferType(const JobGgmlBackend &backend) const noexcept
{
    if (!m_scheduler || !backend.isValid())
        return nullptr;

    return bufferTypeFromNative(ggml_backend_sched_get_buffer_type(m_scheduler.get(), backend.backend()));
}

std::size_t JobGgmlBackendSched::bufferSize(const JobGgmlBackend &backend) const noexcept
{
    if (!m_scheduler || !backend.isValid())
        return 0;

    return ggml_backend_sched_get_buffer_size(m_scheduler.get(), backend.backend());
}

void JobGgmlBackendSched::reserveSize(JobGgmlCGraph &measureGraph, std::vector<std::size_t> &sizes)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot reserve scheduler sizes with an invalid scheduler"
        };
    }

    if (!measureGraph.isValid()) {
        throw std::invalid_argument{
            "reserveSize requires a valid JobGgmlCGraph"
        };
    }

    sizes.resize(static_cast<std::size_t>(backendCount()), 0);

    ggml_backend_sched_reserve_size(m_scheduler.get(), measureGraph.graph(), sizes.data());
}

bool JobGgmlBackendSched::reserve(JobGgmlCGraph &measureGraph)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot reserve scheduler buffers with an invalid scheduler"
        };
    }

    if (!measureGraph.isValid()) {
        throw std::invalid_argument{
            "reserve requires a valid JobGgmlCGraph"
        };
    }

    return ggml_backend_sched_reserve(m_scheduler.get(), measureGraph.graph());
}

void JobGgmlBackendSched::setTensorBackend(JobGgmlTensor &tensor, const JobGgmlBackend &backend)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot assign a tensor with an invalid scheduler"
        };
    }

    if (!tensor.isValid()) {
        throw std::invalid_argument{
            "setTensorBackend requires a valid JobGgmlTensor"
        };
    }

    if (!backend.isValid()) {
        throw std::invalid_argument{
            "setTensorBackend requires a valid JobGgmlBackend"
        };
    }

    if (!backendFromNative(backend.backend())) {
        throw std::invalid_argument{
            "setTensorBackend requires a backend owned by this scheduler"
        };
    }

    ggml_backend_sched_set_tensor_backend(m_scheduler.get(), tensor.tensor(), backend.backend());
}

JobGgmlBackend::Ptr JobGgmlBackendSched::tensorBackend(const JobGgmlTensor &tensor) const noexcept
{
    if (!m_scheduler || !tensor.isValid())
        return nullptr;

    return backendFromNative(ggml_backend_sched_get_tensor_backend(m_scheduler.get(), const_cast<ggml_tensor *>(tensor.tensor())));
}

void JobGgmlBackendSched::splitGraph(JobGgmlCGraph &graph)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot split a graph with an invalid scheduler"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "splitGraph requires a valid JobGgmlCGraph"
        };
    }

    ggml_backend_sched_split_graph(m_scheduler.get(), graph.graph());
}

bool JobGgmlBackendSched::allocateGraph(JobGgmlCGraph &graph)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot allocate a graph with an invalid scheduler"
        };
    }

    if (!graph.isValid()) {
        throw std::invalid_argument{
            "allocateGraph requires a valid JobGgmlCGraph"
        };
    }

    return ggml_backend_sched_alloc_graph(m_scheduler.get(), graph.graph());
}

JobGgmlStatus JobGgmlBackendSched::computeGraph(JobGgmlCGraph &graph)
{
    if (!m_scheduler || !graph.isValid())
        return JobGgmlStatus::Failed;

    return JobGgmlBackend::fromGgmlStatus(ggml_backend_sched_graph_compute(m_scheduler.get(), graph.graph())
        );
}

JobGgmlStatus JobGgmlBackendSched::computeGraphAsync(JobGgmlCGraph &graph)
{
    if (!m_scheduler || !graph.isValid())
        return JobGgmlStatus::Failed;

    return JobGgmlBackend::fromGgmlStatus(ggml_backend_sched_graph_compute_async(m_scheduler.get(), graph.graph()));
}

void JobGgmlBackendSched::synchronize()
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot synchronize an invalid GGML backend scheduler"
        };
    }

    ggml_backend_sched_synchronize(m_scheduler.get());
}

void JobGgmlBackendSched::reset()
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot reset an invalid GGML backend scheduler"
        };
    }

    ggml_backend_sched_reset(m_scheduler.get());
}

void JobGgmlBackendSched::setEvalCallback(EvalCallback callback)
{
    if (!m_scheduler) {
        throw std::runtime_error{
            "Cannot set an evaluation callback on an invalid scheduler"
        };
    }

    m_evalCallback = std::move(callback);
    ggml_backend_sched_set_eval_callback( m_scheduler.get(),
        m_evalCallback ? &JobGgmlBackendSched::evalCallbackTrampoline : nullptr,
        m_evalCallback ? this : nullptr
        );
}

void JobGgmlBackendSched::clearEvalCallback() noexcept
{
    m_evalCallback = {};
    if (m_scheduler)
        ggml_backend_sched_set_eval_callback(m_scheduler.get(), nullptr, nullptr);
}

bool JobGgmlBackendSched::evalCallbackTrampoline(ggml_tensor *tensor, bool ask, void *userData) noexcept
{
    auto *scheduler = static_cast<JobGgmlBackendSched *>(userData);

    if (!scheduler || !scheduler->m_evalCallback || !tensor)
        return false;

    try {
        JobGgmlTensor tensorView{tensor};
        return scheduler->m_evalCallback(tensorView, ask);
    } catch (...) {
        /*
         * IMPORTANT FOR NOW REVISIT LATER
         * Never allow a C++ exception to cross the C callback boundary.
         * Returning false cancels graph computation.
         */
        return false;
    }
}

JobGgmlBackend::Ptr JobGgmlBackendSched::backendFromNative(ggml_backend_t backend) const noexcept
{
    if (!backend)
        return nullptr;

    for (const auto &candidate : m_backends) {
        if (candidate &&
            candidate->backend() == backend) {
            return candidate;
        }
    }

    return nullptr;
}

JobGgmlBackendBufferType::Ptr JobGgmlBackendSched::bufferTypeFromNative(ggml_backend_buffer_type_t bufferType) const noexcept
{
    if (!bufferType)
        return nullptr;

    for (const auto &candidate : m_bufferTypes) {
        if (candidate &&
            candidate->bufferType() == bufferType) {
            return candidate;
        }
    }

    return nullptr;
}

} // namespace job::ggml