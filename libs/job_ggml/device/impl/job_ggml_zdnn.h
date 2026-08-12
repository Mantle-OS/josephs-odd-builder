#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-zdnn.h>
// ggml_backend_is_zdnn() exists in ggml-zdnn.cpp but is not part of the
// public ggml-zdnn.h API. Do not depend on it unless upstream exposes it.

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {
class JOBGGML_EXPORT JobGgmlZdnn : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlZdnn>;
    using WPtr = std::weak_ptr<JobGgmlZdnn>;
    using UPtr = std::unique_ptr<JobGgmlZdnn>;
    explicit JobGgmlZdnn(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlZdnn() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlZdnn>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlZdnn>(device, std::move(backendReg));
    }

    JobGgmlZdnn(const JobGgmlZdnn &) = delete;
    JobGgmlZdnn &operator=(const JobGgmlZdnn &) = delete;
    JobGgmlZdnn(JobGgmlZdnn &&) = delete;
    JobGgmlZdnn &operator=(JobGgmlZdnn &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Zdnn;
    }

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml