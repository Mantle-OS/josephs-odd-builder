#pragma once

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <ggml-sycl.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "backend/job_ggml_backend.h"
#include "backend/job_ggml_backend_buffer_type.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlSycl : public JobGgmlDevice
{
public:
    // SYCL explicitly does not support host memory registration.
    using Ptr  = std::shared_ptr<JobGgmlSycl>;
    using WPtr = std::weak_ptr<JobGgmlSycl>;
    using UPtr = std::unique_ptr<JobGgmlSycl>;

    explicit JobGgmlSycl(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlSycl() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlSycl>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlSycl>(device, std::move(backendReg));
    }

    JobGgmlSycl(const JobGgmlSycl &) = delete;
    JobGgmlSycl &operator=(const JobGgmlSycl &) = delete;
    JobGgmlSycl(JobGgmlSycl &&) = delete;
    JobGgmlSycl &operator=(JobGgmlSycl &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Sycl;
    }

    [[nodiscard]] bool isSyclBackend() const noexcept;

    [[nodiscard]] static int deviceCount() noexcept;
    [[nodiscard]] static std::vector<int> gpuList();

    static void printDevices();

    [[nodiscard]] static std::string deviceDescription(int device);
    [[nodiscard]] static std::size_t deviceFreeMemory(int device) noexcept;
    [[nodiscard]] static std::size_t deviceTotalMemory(int device) noexcept;

    [[nodiscard]] static JobGgmlBackendBufferType::Ptr splitBufferType(std::span<const float> tensorSplit);

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml