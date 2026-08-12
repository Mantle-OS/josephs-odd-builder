#pragma once

#include <memory>
#include <utility>

#include <ggml-backend.h>
#include <ggml-cpp.h>

#include "job_ggml_backend.h"
#include "job_ggml_backend_buffer_type.h"
#include "job_ggml_backend_reg.h"
#include "job_ggml_device_interface.h"
#include "job_ggml_device_props.h"

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDevice>;
    using WPtr = std::weak_ptr<JobGgmlDevice>;
    using UPtr = std::unique_ptr<JobGgmlDevice>;

    explicit JobGgmlDevice(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    virtual ~JobGgmlDevice() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr) { return std::make_shared<JobGgmlDevice>(device, std::move(backendReg)); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr) { return std::make_unique<JobGgmlDevice>( device, std::move(backendReg) ); }

    JobGgmlDevice(const JobGgmlDevice &) = delete;
    JobGgmlDevice &operator=(const JobGgmlDevice &) = delete;
    JobGgmlDevice(JobGgmlDevice &&) = delete;
    JobGgmlDevice &operator=(JobGgmlDevice &&) = delete;

    [[nodiscard]] ggml_backend_dev_t device() const noexcept;

    [[nodiscard]] JobGgmlDeviceInterface *deviceInterface() noexcept;
    [[nodiscard]] const JobGgmlDeviceInterface *deviceInterface() const noexcept;

    std::string uid() const noexcept { return m_props ?  m_props->name() : "unknown"; }

    [[nodiscard]] JobGgmlDeviceProps *props() noexcept;
    [[nodiscard]] const JobGgmlDeviceProps *props() const noexcept;

    [[nodiscard]] JobGgmlDeviceCaps *caps() noexcept;
    [[nodiscard]] const JobGgmlDeviceCaps *caps() const noexcept;

    [[nodiscard]] JobGgmlBackendReg::Ptr backendReg() const noexcept;
    void setBackendReg(JobGgmlBackendReg::Ptr backendReg) noexcept;

    [[nodiscard]] JobGgmlBackend::Ptr backend() const noexcept;

    [[nodiscard]] JobGgmlBackendBufferType *bufferType() noexcept;
    [[nodiscard]] const JobGgmlBackendBufferType *bufferType() const noexcept;

    [[nodiscard]] JobGgmlBackendBufferType *hostBufferType() noexcept;
    [[nodiscard]] const JobGgmlBackendBufferType *hostBufferType() const noexcept;

    [[nodiscard]] bool hasBackend() const noexcept;
    [[nodiscard]] bool hasHostBufferType() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;

    // For the devices/impl.
    [[nodiscard]] virtual JobGgmlDeviceImpl impl() const noexcept;

    [[nodiscard]] virtual std::string dump(){return std::string{};};

private:
    ggml_backend_dev_t                   m_device{nullptr}; // Borrowed from the GGML registry.

    JobGgmlDeviceInterface::UPtr         m_deviceInterface;
    JobGgmlBackendReg::Ptr               m_backendReg;
    JobGgmlDeviceProps::UPtr             m_props;

    JobGgmlBackendBufferType::Ptr        m_bufferType;
    JobGgmlBackendBufferType::Ptr        m_hostBufferType;

    JobGgmlBackend::Ptr                  m_backend;
};

} // namespace job::ggml