#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-opencl.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "backend/job_ggml_backend.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlOpenCl : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOpenCl>;
    using WPtr = std::weak_ptr<JobGgmlOpenCl>;
    using UPtr = std::unique_ptr<JobGgmlOpenCl>;

    explicit JobGgmlOpenCl(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlOpenCl() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlOpenCl>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlOpenCl>(device, std::move(backendReg));
    }

    JobGgmlOpenCl(const JobGgmlOpenCl &) = delete;
    JobGgmlOpenCl &operator=(const JobGgmlOpenCl &) = delete;
    JobGgmlOpenCl(JobGgmlOpenCl &&) = delete;
    JobGgmlOpenCl &operator=(JobGgmlOpenCl &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::OpenCl;
    }

    [[nodiscard]] bool isOpenClBackend() const noexcept;
    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml