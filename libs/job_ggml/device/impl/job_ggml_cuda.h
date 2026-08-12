#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <ggml-cuda.h>

#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

#include "backend/job_ggml_backend.h"
#include "backend/job_ggml_backend_buffer_type.h"
#include "tensor/job_ggml_tensor.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlCuda : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlCuda>;
    using WPtr = std::weak_ptr<JobGgmlCuda>;
    using UPtr = std::unique_ptr<JobGgmlCuda>;

    explicit JobGgmlCuda(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlCuda() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlCuda>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlCuda>(device, std::move(backendReg));
    }

    JobGgmlCuda(const JobGgmlCuda &) = delete;
    JobGgmlCuda &operator=(const JobGgmlCuda &) = delete;
    JobGgmlCuda(JobGgmlCuda &&) = delete;
    JobGgmlCuda &operator=(JobGgmlCuda &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Cuda;
    }

    [[nodiscard]] bool isCudaBackend() const noexcept;
    [[nodiscard]] static int deviceCount() noexcept;
    [[nodiscard]] int cudaDeviceIndex() const noexcept;

    [[nodiscard]] bool canAccessPeer(const JobGgmlCuda &targetDevice) const noexcept;
    [[nodiscard]] bool enablePeerAccess(JobGgmlCuda &targetDevice) noexcept;
    [[nodiscard]] static bool enableBiDirectionalP2P(JobGgmlCuda &dev0, JobGgmlCuda &dev1) noexcept;

    [[nodiscard]] bool copyToPeer(void *dstPtr,
                                  const JobGgmlCuda &dstDevice,
                                  const void *srcPtr,
                                  std::size_t sizeBytes) const noexcept;

    [[nodiscard]] static bool registerHostBuffer(void *buffer, std::size_t size) noexcept;
    static void unregisterHostBuffer(void *buffer) noexcept;

    [[nodiscard]] static JobGgmlBackendBufferType::Ptr splitBufferType(int mainDevice, std::span<const float> tensorSplit);
    [[nodiscard]] static bool allReduceTensor(std::span<const JobGgmlBackend::Ptr> backends, std::span<const JobGgmlTensor::Ptr> tensors);

    [[nodiscard]] std::string dump() override;
};

} // namespace job::ggml