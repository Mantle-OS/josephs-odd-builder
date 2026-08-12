#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <ggml-backend.h>

#include <job_map.h>
#include <job_obj_hash.h>
#include <job_list.h>

#include "job_ggml_backend_sched.h"
#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

// The IMPL devices
#include "job_ggml_cpu.h"

#ifdef JOB_GGML_VULKAN
#include "job_ggml_vulkan.h"
#endif

#ifdef JOB_GGML_CUDA
#include "job_ggml_cuda.h"
#endif

#ifdef JOB_GGML_OPENCL
#include "job_ggml_opencl.h"
#endif

#ifdef JOB_GGML_BLAS
#include "job_ggml_blas.h"
#endif

#ifdef JOB_GGML_HEXAGON
#include "job_ggml_hexagon.h"
#endif

#ifdef JOB_GGML_OPENVINO
#include "job_ggml_openvino.h"
#endif

#ifdef JOB_GGML_SYCL
#include "job_ggml_sycl.h"
#endif

#ifdef JOB_GGML_WEBGPU
#include "job_ggml_webgpu.h"
#endif

#ifdef JOB_GGML_ZDNN
#include "job_ggml_zdnn.h"
#endif

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDeviceManager
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceManager>;
    using WPtr = std::weak_ptr<JobGgmlDeviceManager>;
    using UPtr = std::unique_ptr<JobGgmlDeviceManager>;

    using Devices     = core::JobObjHashFast<JobGgmlDevice::Ptr>;
    using GpuDevices  = core::JobMap<std::string, JobGgmlDevice *>;
    using BackendRegs = core::JobList<JobGgmlBackendReg::Ptr>;


#ifdef JOB_GGML_VULKAN
    using VulkanDevices = core::JobMap<std::string, JobGgmlVulkan *>;
#endif

#ifdef JOB_GGML_CUDA
    using CudaDevices = core::JobMap<std::string, JobGgmlCuda *>;
#endif

#ifdef JOB_GGML_OPENCL
    using OpenClDevices = core::JobMap<std::string, JobGgmlOpenCl *>;
#endif

#ifdef JOB_GGML_BLAS
    using BlasDevices = core::JobMap<std::string, JobGgmlBlas *>;
#endif

#ifdef JOB_GGML_HEXAGON
    using HexagonDevices = core::JobMap<std::string, JobGgmlHexagon *>;
#endif

#ifdef JOB_GGML_OPENVINO
    using OpenVinoDevices = core::JobMap<std::string, JobGgmlOpenVino *>;
#endif

#ifdef JOB_GGML_SYCL
    using SyclDevices = core::JobMap<std::string, JobGgmlSycl *>;
#endif

#ifdef JOB_GGML_WEBGPU
    using WebGpuDevices = core::JobMap<std::string, JobGgmlWebGpu *>;
#endif

#ifdef JOB_GGML_ZDNN
    using ZdnnDevices = core::JobMap<std::string, JobGgmlZdnn *>;
#endif

    explicit JobGgmlDeviceManager(bool scan = true);
    ~JobGgmlDeviceManager() = default;

    [[nodiscard]] static Ptr createShared(bool scan = true)
    {
        return std::make_shared<JobGgmlDeviceManager>(scan);
    }

    [[nodiscard]] static UPtr createUniq(bool scan = true)
    {
        return std::make_unique<JobGgmlDeviceManager>(scan);
    }

    JobGgmlDeviceManager(const JobGgmlDeviceManager &) = delete;
    JobGgmlDeviceManager &operator=(const JobGgmlDeviceManager &) = delete;
    JobGgmlDeviceManager(JobGgmlDeviceManager &&) = delete;
    JobGgmlDeviceManager &operator=(JobGgmlDeviceManager &&) = delete;

    void scan();

    [[nodiscard]] DeviceManagerState state() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool hasCpu() const noexcept;
    [[nodiscard]] bool hasGpu() const noexcept;
    [[nodiscard]] bool hasScheduler() const noexcept;

    [[nodiscard]] const std::string &errorString() const noexcept;

    [[nodiscard]] std::size_t deviceCount() const noexcept;
    [[nodiscard]] std::size_t gpuDeviceCount() const noexcept;

    [[nodiscard]] const Devices &devices() const noexcept;

    [[nodiscard]] JobGgmlDevice *device(const std::string &uid);
    [[nodiscard]] JobGgmlDevice *device(std::size_t idx);

    [[nodiscard]] JobGgmlCpu *cpu();
    [[nodiscard]] GpuDevices fallbackGpus();

    [[nodiscard]] JobGgmlBackendSched::Ptr scheduler() const noexcept;
    void resetScheduler() noexcept;
    [[nodiscard]] std::vector<JobGgmlBackendSched::Ptr> buildScheduler(const std::vector<std::string> &uids,
                                                                       const std::vector<std::size_t> &graphSizes,
                                                                       const std::vector<bool> &parallelFlags,
                                                                       const std::vector<bool> &opOffloadFlags);
    [[nodiscard]] JobGgmlBackendSched::Ptr buildScheduler(const std::string &uid,
                                                          std::size_t graphSize =  GGML_DEFAULT_GRAPH_SIZE,
                                                          bool parallel = false,
                                                          bool opOffload = true);
    [[nodiscard]] JobGgmlBackendSched::Ptr buildScheduler(JobGgmlDevice *dev,
                                                          std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                                          bool parallel = false,
                                                          bool opOffload = true);

    void reset() noexcept;
    [[nodiscard]] std::string debugString() const;

#ifdef JOB_GGML_VULKAN
    [[nodiscard]] VulkanDevices vulkanDevices();
    [[nodiscard]] JobGgmlVulkan *vulkan(const std::string &uid);
    [[nodiscard]] JobGgmlVulkan *vulkan(std::size_t idx);
    [[nodiscard]] bool hasVulkan() const noexcept;
#endif

#ifdef JOB_GGML_CUDA
    [[nodiscard]] CudaDevices cudaDevices();
    [[nodiscard]] JobGgmlCuda *cuda(const std::string &uid);
    [[nodiscard]] JobGgmlCuda *cuda(std::size_t idx);
    [[nodiscard]] bool hasCuda() const noexcept;
#endif

#ifdef JOB_GGML_OPENCL
    [[nodiscard]] OpenClDevices openClDevices();
    [[nodiscard]] JobGgmlOpenCl *openCl(const std::string &uid);
    [[nodiscard]] JobGgmlOpenCl *openCl(std::size_t idx);
    [[nodiscard]] bool hasOpenCl() const noexcept;
#endif

#ifdef JOB_GGML_BLAS
    [[nodiscard]] BlasDevices blasDevices();
    [[nodiscard]] JobGgmlBlas *blas(const std::string &uid);
    [[nodiscard]] JobGgmlBlas *blas(std::size_t idx);
    [[nodiscard]] bool hasBlas() const noexcept;
#endif

#ifdef JOB_GGML_HEXAGON
    [[nodiscard]] HexagonDevices hexagonDevices();
    [[nodiscard]] JobGgmlHexagon *hexagon(const std::string &uid);
    [[nodiscard]] JobGgmlHexagon *hexagon(std::size_t idx);
    [[nodiscard]] bool hasHexagon() const noexcept;
#endif

#ifdef JOB_GGML_OPENVINO
    [[nodiscard]] OpenVinoDevices openVinoDevices();
    [[nodiscard]] JobGgmlOpenVino *openVino(const std::string &uid);
    [[nodiscard]] JobGgmlOpenVino *openVino(std::size_t idx);
    [[nodiscard]] bool hasOpenVino() const noexcept;
#endif

#ifdef JOB_GGML_SYCL
    [[nodiscard]] SyclDevices syclDevices();
    [[nodiscard]] JobGgmlSycl *sycl(const std::string &uid);
    [[nodiscard]] JobGgmlSycl *sycl(std::size_t idx);
    [[nodiscard]] bool hasSycl() const noexcept;
#endif

#ifdef JOB_GGML_WEBGPU
    [[nodiscard]] WebGpuDevices webGpuDevices();
    [[nodiscard]] JobGgmlWebGpu *webGpu(const std::string &uid);
    [[nodiscard]] JobGgmlWebGpu *webGpu(std::size_t idx);
    [[nodiscard]] bool hasWebGpu() const noexcept;
#endif

#ifdef JOB_GGML_ZDNN
    [[nodiscard]] ZdnnDevices zdnnDevices();
    [[nodiscard]] JobGgmlZdnn *zdnn(const std::string &uid);
    [[nodiscard]] JobGgmlZdnn *zdnn(std::size_t idx);
    [[nodiscard]] bool hasZdnn() const noexcept;
#endif

private:
    void setState(DeviceManagerState state) noexcept;
    void setErrorString(const std::string &errorString);
    void clearError() noexcept;

    [[nodiscard]] JobGgmlBackendReg::Ptr registryFromNative(ggml_backend_reg_t backendReg) const noexcept;

    void indexDevice(const std::string &uid, JobGgmlDeviceImpl impl, const JobGgmlDevice::Ptr &device);
    void unindexDevice(const std::string &uid, JobGgmlDeviceImpl impl) noexcept;

    DeviceManagerState                  m_state{DeviceManagerState::Uninitialized};
    std::string                         m_errorString;

    Devices                             m_devices;
    JobGgmlCpu::Ptr                     m_cpuDevice;
    GpuDevices                          m_fallbackGpus;

#ifdef JOB_GGML_VULKAN
    VulkanDevices                       m_vulkanDevices;
#endif

#ifdef JOB_GGML_CUDA
    CudaDevices                         m_cudaDevices;
#endif

#ifdef JOB_GGML_OPENCL
    OpenClDevices                       m_openClDevices;
#endif

#ifdef JOB_GGML_BLAS
    BlasDevices                         m_blasDevices;
#endif

#ifdef JOB_GGML_HEXAGON
    HexagonDevices                      m_hexagonDevices;
#endif

#ifdef JOB_GGML_OPENVINO
    OpenVinoDevices                     m_openVinoDevices;
#endif

#ifdef JOB_GGML_SYCL
    SyclDevices                         m_syclDevices;
#endif

#ifdef JOB_GGML_WEBGPU
    WebGpuDevices                       m_webGpuDevices;
#endif

#ifdef JOB_GGML_ZDNN
    ZdnnDevices                         m_zdnnDevices;
#endif

    BackendRegs                         m_backendRegistries;
    JobGgmlBackendSched::Ptr            m_scheduler;    
};

} // namespace job::ggml