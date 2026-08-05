#include "job_ggml_device_interface.h"

#include <stdexcept>
#include <utility>
namespace job::ggml {

JobGgmlDeviceInterface::JobGgmlDeviceInterface(ggml_backend_dev_t device) :
    m_device{device}
{
    if (!m_device) {
        throw std::invalid_argument{
            "JobGgmlDeviceInterface requires a valid ggml_backend_dev_t"
        };
    }
}

bool JobGgmlDeviceInterface::isValid() const noexcept
{
    return m_device != nullptr;
}

std::string JobGgmlDeviceInterface::name() const
{
    if (!m_device)
        return {};

    const char *nativeName = ggml_backend_dev_name(m_device);

    return nativeName ? std::string{nativeName} : std::string{};
}

std::string JobGgmlDeviceInterface::description() const
{
    if (!m_device)
        return {};

    const char *nativeDescription = ggml_backend_dev_description( m_device );
    return nativeDescription ? std::string{nativeDescription} : std::string{};
}

void JobGgmlDeviceInterface::memory(std::size_t &freeMemory, std::size_t &totalMemory) const noexcept
{
    freeMemory  = 0;
    totalMemory = 0;

    if (!m_device)
        return;

    ggml_backend_dev_memory(m_device, &freeMemory, &totalMemory);
}

enum ggml_backend_dev_type JobGgmlDeviceInterface::type() const noexcept
{
    return m_device ? ggml_backend_dev_type(m_device) : GGML_BACKEND_DEVICE_TYPE_CPU;
}

JobGgmlDeviceProps::UPtr JobGgmlDeviceInterface::props() const
{
    if (!m_device) {
        throw std::runtime_error{
            "Cannot query properties from an invalid GGML device interface"
        };
    }

    ggml_backend_dev_props nativeProps{};
    ggml_backend_dev_get_props(m_device, &nativeProps);
    return JobGgmlDeviceProps::createUniq(nativeProps);
}

JobGgmlBackend::UPtr JobGgmlDeviceInterface::initBackend(const std::string &params) const
{
    if (!m_device) {
        throw std::runtime_error{
            "Cannot initialize a backend from an invalid GGML device interface"
        };
    }

    ggml_backend_ptr nativeBackend{
        ggml_backend_dev_init(
            m_device,
            params.empty() ? nullptr : params.c_str()
            )
    };

    if (!nativeBackend) {
        throw std::runtime_error{
            "Failed to initialize GGML backend"
        };
    }

    return JobGgmlBackend::createUniq(std::move(nativeBackend));
}

JobGgmlBackendBufferType::Ptr JobGgmlDeviceInterface::bufferType() const
{
    if (!m_device)
        return nullptr;

    ggml_backend_buffer_type_t nativeBufferType = ggml_backend_dev_buffer_type(m_device);
    return nativeBufferType ? JobGgmlBackendBufferType::createShared( nativeBufferType ) : nullptr;
}

JobGgmlBackendBufferType::Ptr JobGgmlDeviceInterface::hostBufferType() const
{
    if (!m_device)
        return nullptr;

    ggml_backend_buffer_type_t nativeBufferType = ggml_backend_dev_host_buffer_type(m_device);
    return nativeBufferType ? JobGgmlBackendBufferType::createShared(nativeBufferType) : nullptr;
}

JobGgmlBackendBuffer::UPtr JobGgmlDeviceInterface::bufferFromHostPtr(void *ptr, std::size_t size, std::size_t maxTensorSize) const
{
    if (!m_device) {
        throw std::runtime_error{
            "Cannot create a host-backed buffer from an invalid GGML device interface"
        };
    }

    if (!ptr) {
        throw std::invalid_argument{
            "bufferFromHostPtr requires a valid host pointer"
        };
    }

    if (size == 0) {
        throw std::invalid_argument{
            "bufferFromHostPtr requires a size greater than zero"
        };
    }

    ggml_backend_buffer_ptr nativeBuffer{
        ggml_backend_dev_buffer_from_host_ptr(
            m_device,
            ptr,
            size,
            maxTensorSize
            )
    };

    if (!nativeBuffer) {
        throw std::runtime_error{
            "Failed to create a GGML backend buffer from the host pointer"
        };
    }

    return JobGgmlBackendBuffer::createUniq(
        std::move(nativeBuffer)
        );
}

bool JobGgmlDeviceInterface::supportsOp(const JobGgmlTensor &operation) const noexcept
{
    return m_device && operation.isValid() && ggml_backend_dev_supports_op(m_device, operation.tensor());
}

bool JobGgmlDeviceInterface::supportsBufferType(const JobGgmlBackendBufferType &bufferType) const noexcept
{
    return m_device && bufferType.isValid() && ggml_backend_dev_supports_buft(m_device, bufferType.bufferType());
}

bool JobGgmlDeviceInterface::offloadOp(const JobGgmlTensor &operation) const noexcept
{
    return m_device && operation.isValid() && ggml_backend_dev_offload_op(m_device, operation.tensor());
}

JobGgmlBackendEvent::UPtr JobGgmlDeviceInterface::createEvent() const
{
    if (!m_device) {
        throw std::runtime_error{
            "Cannot create an event from an invalid GGML device interface"
        };
    }

    /*
     * JobGgmlBackendEvent creates and owns the native event associated with
     * this borrowed device.
     */
    return JobGgmlBackendEvent::createUniq(m_device);
}

void JobGgmlDeviceInterface::synchronizeEvent(JobGgmlBackendEvent &event) const
{
    if (!m_device) {
        throw std::runtime_error{
            "Cannot synchronize an event with an invalid GGML device interface"
        };
    }

    if (!event.isValid()) {
        throw std::invalid_argument{
            "synchronizeEvent requires a valid JobGgmlBackendEvent"
        };
    }

    event.synchronize();
}

ggml_backend_dev_t JobGgmlDeviceInterface::device() const noexcept
{
    return m_device;
}

} // namespace job::ggml