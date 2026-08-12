#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-webgpu.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

// Needed for examples in ggml
// GGML_BACKEND_API ggml_backend_t ggml_backend_webgpu_init(void); // Look at example later REFACTOR

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlWebGpu : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlWebGpu>;
    using WPtr = std::weak_ptr<JobGgmlWebGpu>;
    using UPtr = std::unique_ptr<JobGgmlWebGpu>;

    explicit JobGgmlWebGpu(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlWebGpu() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlWebGpu>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlWebGpu>(device, std::move(backendReg));
    }

    JobGgmlWebGpu(const JobGgmlWebGpu &) = delete;
    JobGgmlWebGpu &operator=(const JobGgmlWebGpu &) = delete;
    JobGgmlWebGpu(JobGgmlWebGpu &&) = delete;
    JobGgmlWebGpu &operator=(JobGgmlWebGpu &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::WebGpu;
    }

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml