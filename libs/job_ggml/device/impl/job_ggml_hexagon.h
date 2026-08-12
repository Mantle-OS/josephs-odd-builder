#pragma once

#include <memory>
#include <string>
#include <utility>

#include <ggml-hexagon.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

// C++ Object Wrappers
#include "backend/job_ggml_backend.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlHexagon : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlHexagon>;
    using WPtr = std::weak_ptr<JobGgmlHexagon>;
    using UPtr = std::unique_ptr<JobGgmlHexagon>;

    explicit JobGgmlHexagon(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlHexagon() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlHexagon>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlHexagon>(device, std::move(backendReg));
    }

    JobGgmlHexagon(const JobGgmlHexagon &) = delete;
    JobGgmlHexagon &operator=(const JobGgmlHexagon &) = delete;
    JobGgmlHexagon(JobGgmlHexagon &&) = delete;
    JobGgmlHexagon &operator=(JobGgmlHexagon &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Hexagon;
    }

    [[nodiscard]] bool isHexagonBackend() const noexcept;

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml