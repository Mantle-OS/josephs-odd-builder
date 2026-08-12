#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <ggml-backend.h>

#include "job_ggml_backend.h"
#include "job_ggml_backend_buffer.h"
#include "job_ggml_backend_buffer_type.h"
#include "job_ggml_backend_event.h"
#include "job_ggml_device_props.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDeviceInterface
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceInterface>;
    using WPtr = std::weak_ptr<JobGgmlDeviceInterface>;
    using UPtr = std::unique_ptr<JobGgmlDeviceInterface>;

    explicit JobGgmlDeviceInterface(ggml_backend_dev_t device);

    ~JobGgmlDeviceInterface() = default;
    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device)
    {
        return std::make_shared<JobGgmlDeviceInterface>(device);
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device)
    {
        return std::make_unique<JobGgmlDeviceInterface>(device);
    }

    JobGgmlDeviceInterface(const JobGgmlDeviceInterface &) = delete;
    JobGgmlDeviceInterface &operator=(const JobGgmlDeviceInterface &) = delete;
    JobGgmlDeviceInterface(JobGgmlDeviceInterface &&) = delete;
    JobGgmlDeviceInterface &operator=(JobGgmlDeviceInterface &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::string description() const;

    void memory(std::size_t &freeMemory, std::size_t &totalMemory) const noexcept;

    [[nodiscard]] JobGgmlDeviceType type() const noexcept;
    [[nodiscard]] JobGgmlDeviceProps::UPtr props() const;

    [[nodiscard]] JobGgmlBackend::UPtr initBackend(const std::string &params = {}) const;

    [[nodiscard]] JobGgmlBackendBufferType::Ptr bufferType() const;
    [[nodiscard]] JobGgmlBackendBufferType::Ptr hostBufferType() const;

    // The returned JobGgmlBackendBuffer owns the native backend-buffer object,
    // The supplied host memory remains owned by the caller and must remain valid for the lifetime required by the backend buffer.
    [[nodiscard]] JobGgmlBackendBuffer::UPtr bufferFromHostPtr(void *ptr, std::size_t size, std::size_t maxTensorSize) const;

    [[nodiscard]] bool supportsOp(const JobGgmlTensor &operation) const noexcept;
    [[nodiscard]] bool supportsBufferType(const JobGgmlBackendBufferType &bufferType) const noexcept;
    [[nodiscard]] bool offloadOp(const JobGgmlTensor &operation) const noexcept;

    [[nodiscard]] JobGgmlBackendEvent::UPtr createEvent() const;
    void synchronizeEvent(JobGgmlBackendEvent &event) const;

    [[nodiscard]] ggml_backend_dev_t device() const noexcept;     // hatch

private:
    ggml_backend_dev_t m_device{nullptr}; // Borrowed from the GGML registry.
};

} // namespace job::ggml