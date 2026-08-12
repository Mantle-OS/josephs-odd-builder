#pragma once

#include <memory>
#include <utility>
#include <sstream>

#include <ggml-cpu.h>

#include "job_ggml_abort_callback.h"
#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "job_ggml_threadpool.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlCpu : public JobGgmlDevice
{
public:
    using Ptr  = std::shared_ptr<JobGgmlCpu>;
    using WPtr = std::weak_ptr<JobGgmlCpu>;
    using UPtr = std::unique_ptr<JobGgmlCpu>;

    explicit JobGgmlCpu(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr);
    ~JobGgmlCpu() override = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_shared<JobGgmlCpu>(device, std::move(backendReg));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg = nullptr)
    {
        return std::make_unique<JobGgmlCpu>(device, std::move(backendReg));
    }

    JobGgmlCpu(const JobGgmlCpu &) = delete;
    JobGgmlCpu &operator=(const JobGgmlCpu &) = delete;
    JobGgmlCpu(JobGgmlCpu &&) = delete;
    JobGgmlCpu &operator=(JobGgmlCpu &&) = delete;

    [[nodiscard]] JobGgmlDeviceImpl impl() const noexcept override
    {
        return JobGgmlDeviceImpl::Cpu;
    }

    [[nodiscard]] bool isCpuBackend() const noexcept;
    void setNThreads(int nThreads);

    void setThreadPool(JobGgmlThreadPool *threadPool);
    void setAbortCallback(JobGgmlAbortCallback *callback);

    void setUseReference(bool useReference);

    static void initializeNuma(JobGgmlNumaStrategy strategy);
    [[nodiscard]] static bool isNuma() noexcept;

    [[nodiscard]] static bool hasSse3() noexcept;
    [[nodiscard]] static bool hasSsse3() noexcept;
    [[nodiscard]] static bool hasAvx() noexcept;
    [[nodiscard]] static bool hasAvxVnni() noexcept;
    [[nodiscard]] static bool hasAvx2() noexcept;
    [[nodiscard]] static bool hasBmi2() noexcept;
    [[nodiscard]] static bool hasF16c() noexcept;
    [[nodiscard]] static bool hasFma() noexcept;
    [[nodiscard]] static bool hasAvx512() noexcept;
    [[nodiscard]] static bool hasAvx512Vbmi() noexcept;
    [[nodiscard]] static bool hasAvx512Vnni() noexcept;
    [[nodiscard]] static bool hasAvx512Bf16() noexcept;
    [[nodiscard]] static bool hasAmxInt8() noexcept;

    [[nodiscard]] static bool hasNeon() noexcept;
    [[nodiscard]] static bool hasArmFma() noexcept;
    [[nodiscard]] static bool hasFp16Va() noexcept;
    [[nodiscard]] static bool hasDotProd() noexcept;
    [[nodiscard]] static bool hasMatMulInt8() noexcept;
    [[nodiscard]] static bool hasSve() noexcept;
    [[nodiscard]] static int  sveCount() noexcept;
    [[nodiscard]] static bool hasSme() noexcept;

    [[nodiscard]] static bool hasRiscvV() noexcept;
    [[nodiscard]] static int  rvvVectorLength() noexcept;
    [[nodiscard]] static bool hasVsx() noexcept;
    [[nodiscard]] static bool hasVxe() noexcept;
    [[nodiscard]] static bool hasWasmSimd() noexcept;
    [[nodiscard]] static bool hasLlamaFile() noexcept;


    // for the manfers debug output
    [[nodiscard]] std::string dump() override;

};
} // namespace job::ggml