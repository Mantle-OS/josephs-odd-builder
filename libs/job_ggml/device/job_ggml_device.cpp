#include "job_ggml_device.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlDevice::JobGgmlDevice(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    m_device{device},
    m_backendReg{std::move(backendReg)}
{
    if (!m_device) {
        throw std::invalid_argument{
            "JobGgmlDevice requires a valid ggml_backend_dev_t"
        };
    }

    m_deviceInterface = JobGgmlDeviceInterface::createUniq(m_device);
    m_props = m_deviceInterface->props();

    if (!m_props) {
        throw std::runtime_error{
            "Failed to retrieve GGML device properties"
        };
    }

    if (!m_backendReg) {
        const ggml_backend_reg_t nativeReg = ggml_backend_dev_backend_reg(m_device);

        if (nativeReg)
            m_backendReg =JobGgmlBackendReg::createShared(nativeReg);
    }

    m_bufferType = m_deviceInterface->bufferType();

    if (!m_bufferType) {
        throw std::runtime_error{
            "GGML device does not provide a preferred buffer type"
        };
    }

    // Host buffer support is optional. A null shared pointer is valid here.
    m_hostBufferType = m_deviceInterface->hostBufferType();
    JobGgmlBackend::UPtr initializedBackend =
        m_deviceInterface->initBackend();

    if (!initializedBackend) {
        throw std::runtime_error{
            "Failed to initialize the GGML device backend"
        };
    }

    m_backend = JobGgmlBackend::Ptr{std::move(initializedBackend)};
}

ggml_backend_dev_t JobGgmlDevice::device() const noexcept
{
    return m_device;
}

JobGgmlDeviceInterface *JobGgmlDevice::deviceInterface() noexcept
{
    return m_deviceInterface.get();
}

const JobGgmlDeviceInterface *JobGgmlDevice::deviceInterface() const noexcept
{
    return m_deviceInterface.get();
}

JobGgmlDeviceProps *JobGgmlDevice::props() noexcept
{
    return m_props.get();
}

const JobGgmlDeviceProps *JobGgmlDevice::props() const noexcept
{
    return m_props.get();
}

JobGgmlDeviceCaps *JobGgmlDevice::caps() noexcept
{
    return m_props ? m_props->caps() : nullptr;
}

const JobGgmlDeviceCaps *JobGgmlDevice::caps() const noexcept
{
    return m_props ? m_props->caps() : nullptr;
}

JobGgmlBackendReg::Ptr JobGgmlDevice::backendReg() const noexcept
{
    return m_backendReg;
}

void JobGgmlDevice::setBackendReg(JobGgmlBackendReg::Ptr backendReg) noexcept
{
    if (m_backendReg != backendReg)
        m_backendReg = std::move(backendReg);
}

JobGgmlBackend::Ptr JobGgmlDevice::backend() const noexcept
{
    return m_backend;
}

JobGgmlBackendBufferType *JobGgmlDevice::bufferType() noexcept
{
    return m_bufferType.get();
}

const JobGgmlBackendBufferType *JobGgmlDevice::bufferType() const noexcept
{
    return m_bufferType.get();
}

JobGgmlBackendBufferType *JobGgmlDevice::hostBufferType() noexcept
{
    return m_hostBufferType.get();
}

const JobGgmlBackendBufferType *JobGgmlDevice::hostBufferType() const noexcept
{
    return m_hostBufferType.get();
}

bool JobGgmlDevice::hasBackend() const noexcept
{
    return m_backend && m_backend->isValid();
}

bool JobGgmlDevice::hasHostBufferType() const noexcept
{
    return m_hostBufferType != nullptr;
}

bool JobGgmlDevice::isValid() const noexcept
{
    return m_device != nullptr &&
           m_deviceInterface &&
           m_deviceInterface->isValid() &&
           m_props &&
           m_bufferType &&
           m_backend &&
           m_backend->isValid();
}


} // namespace job::ggml