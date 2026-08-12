#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <optional>

#include <ggml-openvino.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "backend/job_ggml_backend.h"
#include "backend/job_ggml_backend_buffer.h"
#include "backend/job_ggml_backend_buffer_type.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlOpenVino : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOpenVino>;
    using WPtr = std::weak_ptr<JobGgmlOpenVino>;
    using UPtr = std::unique_ptr<JobGgmlOpenVino>;

    explicit JobGgmlOpenVino(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlOpenVino() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlOpenVino>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlOpenVino>(device, std::move(backendReg));
    }

    JobGgmlOpenVino(const JobGgmlOpenVino &) = delete;
    JobGgmlOpenVino &operator=(const JobGgmlOpenVino &) = delete;
    JobGgmlOpenVino(JobGgmlOpenVino &&) = delete;
    JobGgmlOpenVino &operator=(JobGgmlOpenVino &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::OpenVino;
    }

    [[nodiscard]] bool isOpenVinoBackend() const noexcept;

    [[nodiscard]] static int deviceCount() noexcept;

    [[nodiscard]] static bool isOpenVinoBuffer(const JobGgmlBackendBuffer &buffer) noexcept;
    [[nodiscard]] static bool isOpenVinoBufferType(const JobGgmlBackendBufferType &bufferType) noexcept;
    [[nodiscard]] static bool isOpenVinoHostBufferType(const JobGgmlBackendBufferType &bufferType) noexcept;

    [[nodiscard]] static std::optional<std::size_t> bufferContextId(const JobGgmlBackendBuffer &buffer) noexcept;

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml