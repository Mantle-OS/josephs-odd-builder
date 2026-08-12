#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-blas.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "backend/job_ggml_backend.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBlas : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBlas>;
    using WPtr = std::weak_ptr<JobGgmlBlas>;
    using UPtr = std::unique_ptr<JobGgmlBlas>;

    explicit JobGgmlBlas(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlBlas() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlBlas>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlBlas>(device, std::move(backendReg));
    }

    JobGgmlBlas(const JobGgmlBlas &) = delete;
    JobGgmlBlas &operator=(const JobGgmlBlas &) = delete;
    JobGgmlBlas(JobGgmlBlas &&) = delete;
    JobGgmlBlas &operator=(JobGgmlBlas &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Blas;
    }

    [[nodiscard]] bool isBlasBackend() const noexcept;
    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml