#include "job_ggml_backend.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlBackend::JobGgmlBackend(ggml_backend_t backend) :
    m_backend{backend}
{
    if (!m_backend)
        throw std::invalid_argument{"JobGgmlBackend requires a valid ggml_backend_t"};

    const char *backendName = ggml_backend_name(m_backend.get());
    setName(backendName ? backendName : "unknown");
}

JobGgmlBackend::JobGgmlBackend(ggml_backend_ptr backend) :
    m_backend{std::move(backend)}
{
    if (!m_backend)
        throw std::invalid_argument{"JobGgmlBackend requires a valid ggml_backend_ptr"};

    const char *backendName = ggml_backend_name(m_backend.get());
    setName(backendName ? backendName : "unknown");
}

const std::string &JobGgmlBackend::name() const noexcept
{
    return m_name;
}

void JobGgmlBackend::setName(const std::string &name)
{
    if (m_name != name && !name.empty())
        m_name = name;
}

ggml_backend_t JobGgmlBackend::backend() const noexcept
{
    return m_backend.get();
}

ggml_backend_dev_t JobGgmlBackend::device() const noexcept
{
    return m_backend ? ggml_backend_get_device(m_backend.get()) : nullptr;
}

bool JobGgmlBackend::isValid() const noexcept
{
    return m_backend != nullptr;
}

bool JobGgmlBackend::isCpu() const noexcept
{
    const ggml_backend_dev_t backendDevice = device();

    return backendDevice &&
           ggml_backend_dev_type(backendDevice) == GGML_BACKEND_DEVICE_TYPE_CPU;
}

void JobGgmlBackend::setTensorAsync(JobGgmlTensor &tensor, const void *data, std::size_t offset, std::size_t size)
{
    if (!m_backend) {
        throw std::runtime_error{
            "Cannot set tensor data with an invalid GGML backend"
        };
    }

    if (!tensor.isValid()) {
        throw std::invalid_argument{
            "setTensorAsync requires a valid JobGgmlTensor"
        };
    }

    if (!data && size > 0) {
        throw std::invalid_argument{
            "setTensorAsync requires valid data when size is greater than zero"
        };
    }

    ggml_backend_tensor_set_async(
        m_backend.get(),
        tensor.tensor(),
        data,
        offset,
        size
        );
}

void JobGgmlBackend::getTensorAsync(const JobGgmlTensor &tensor, void *data, std::size_t offset, std::size_t size)
{
    if (!m_backend) {
        throw std::runtime_error{
            "Cannot get tensor data with an invalid GGML backend"
        };
    }

    if (!tensor.isValid()) {
        throw std::invalid_argument{
            "getTensorAsync requires a valid JobGgmlTensor"
        };
    }

    if (!data && size > 0) {
        throw std::invalid_argument{
            "getTensorAsync requires valid output storage when size is greater than zero"
        };
    }

    ggml_backend_tensor_get_async(
        m_backend.get(),
        tensor.tensor(),
        data,
        offset,
        size
        );
}

void JobGgmlBackend::setTensor2dAsync(
    JobGgmlTensor &tensor,
    const void *data,
    std::size_t offset,
    std::size_t size,
    std::size_t copies,
    std::size_t tensorStride,
    std::size_t dataStride
    )
{
    if (!m_backend) {
        throw std::runtime_error{
            "Cannot set 2D tensor data with an invalid GGML backend"
        };
    }

    if (!tensor.isValid()) {
        throw std::invalid_argument{
            "setTensor2dAsync requires a valid JobGgmlTensor"
        };
    }

    if (!data && size > 0) {
        throw std::invalid_argument{
            "setTensor2dAsync requires valid data when size is greater than zero"
        };
    }

    ggml_backend_tensor_set_2d_async(
        m_backend.get(),
        tensor.tensor(),
        data,
        offset,
        size,
        copies,
        tensorStride,
        dataStride
        );
}

void JobGgmlBackend::getTensor2dAsync(
    const JobGgmlTensor &tensor,
    void *data,
    std::size_t offset,
    std::size_t size,
    std::size_t copies,
    std::size_t tensorStride,
    std::size_t dataStride
    )
{
    if (!m_backend) {
        throw std::runtime_error{
            "Cannot get 2D tensor data with an invalid GGML backend"
        };
    }

    if (!tensor.isValid()) {
        throw std::invalid_argument{
            "getTensor2dAsync requires a valid JobGgmlTensor"
        };
    }

    if (!data && size > 0) {
        throw std::invalid_argument{
            "getTensor2dAsync requires valid output storage when size is greater than zero"
        };
    }

    ggml_backend_tensor_get_2d_async(
        m_backend.get(),
        tensor.tensor(),
        data,
        offset,
        size,
        copies,
        tensorStride,
        dataStride
        );
}

void JobGgmlBackend::copyTensorAsync(JobGgmlBackend &destination, const JobGgmlTensor &source, JobGgmlTensor &target)
{
    if (!m_backend) {
        throw std::runtime_error{
            "Cannot copy a tensor from an invalid GGML backend"
        };
    }

    if (!destination.isValid()) {
        throw std::invalid_argument{
            "copyTensorAsync requires a valid destination backend"
        };
    }

    if (!source.isValid()) {
        throw std::invalid_argument{
            "copyTensorAsync requires a valid source tensor"
        };
    }

    if (!target.isValid()) {
        throw std::invalid_argument{
            "copyTensorAsync requires a valid target tensor"
        };
    }

    ggml_backend_tensor_copy_async(
        m_backend.get(),
        destination.backend(),
        source.tensor(),
        target.tensor()
        );
}

void JobGgmlBackend::synchronize()
{
    if (!m_backend)
        throw std::runtime_error{"Cannot synchronize an invalid GGML backend"};

    ggml_backend_synchronize(m_backend.get());
}

ggml_backend_graph_plan_t JobGgmlBackend::createGraphPlan(
    JobGgmlCGraph &graph
    )
{
    if (!m_backend)
        throw std::runtime_error{"Cannot create a graph plan with an invalid GGML backend"};

    if (!graph.isValid())
        throw std::invalid_argument{"createGraphPlan requires a valid JobGgmlCGraph"};

    ggml_backend_graph_plan_t plan = ggml_backend_graph_plan_create(
        m_backend.get(),
        graph.graph()
        );

    if (!plan)
        throw std::runtime_error{"Failed to create GGML backend graph plan"};

    return plan;
}

void JobGgmlBackend::freeGraphPlan(
    ggml_backend_graph_plan_t plan
    ) noexcept
{
    if (m_backend && plan)
        ggml_backend_graph_plan_free(m_backend.get(), plan);
}

JobGgmlStatus JobGgmlBackend::computeGraphPlan(
    ggml_backend_graph_plan_t plan
    )
{
    if (!m_backend || !plan)
        return JobGgmlStatus::Failed;

    return fromGgmlStatus(
        ggml_backend_graph_plan_compute(
            m_backend.get(),
            plan
            )
        );
}

JobGgmlStatus JobGgmlBackend::computeGraph(
    JobGgmlCGraph &graph
    )
{
    if (!m_backend || !graph.isValid())
        return JobGgmlStatus::Failed;

    return fromGgmlStatus(
        ggml_backend_graph_compute(
            m_backend.get(),
            graph.graph()
            )
        );
}

JobGgmlStatus JobGgmlBackend::computeGraphAsync(
    JobGgmlCGraph &graph
    )
{
    if (!m_backend || !graph.isValid())
        return JobGgmlStatus::Failed;

    return fromGgmlStatus(
        ggml_backend_graph_compute_async(
            m_backend.get(),
            graph.graph()
            )
        );
}

void JobGgmlBackend::recordEvent(JobGgmlBackendEvent &event)
{
    if (!m_backend)
        throw std::runtime_error{
            "Cannot record an event with an invalid GGML backend"
        };

    if (!event.isValid())
        throw std::invalid_argument{
            "recordEvent requires a valid JobGgmlBackendEvent"
        };

    ggml_backend_event_record(
        event.event(),
        m_backend.get()
        );
}

void JobGgmlBackend::waitEvent(JobGgmlBackendEvent &event)
{
    if (!m_backend)
        throw std::runtime_error{
            "Cannot wait for an event with an invalid GGML backend"
        };

    if (!event.isValid())
        throw std::invalid_argument{
            "waitEvent requires a valid JobGgmlBackendEvent"
        };

    ggml_backend_event_wait(
        m_backend.get(),
        event.event()
        );
}

} // namespace job::ggml