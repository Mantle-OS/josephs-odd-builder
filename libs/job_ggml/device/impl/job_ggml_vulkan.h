#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-vulkan.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlVulkan : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlVulkan>;
    using WPtr = std::weak_ptr<JobGgmlVulkan>;
    using UPtr = std::unique_ptr<JobGgmlVulkan>;

    explicit JobGgmlVulkan(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);

    ~JobGgmlVulkan() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlVulkan>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlVulkan>(device, std::move(backendReg));
    }

    JobGgmlVulkan(const JobGgmlVulkan &) = delete;
    JobGgmlVulkan &operator=(const JobGgmlVulkan &) = delete;
    JobGgmlVulkan(JobGgmlVulkan &&) = delete;
    JobGgmlVulkan &operator=(JobGgmlVulkan &&) = delete;
    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Vulkan;
    }

    [[nodiscard]] bool isVulkanBackend() const noexcept;
    [[nodiscard]] static int deviceCount() noexcept;
    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml