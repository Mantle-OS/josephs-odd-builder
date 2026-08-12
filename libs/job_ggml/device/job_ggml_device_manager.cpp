#include "job_ggml_device_manager.h"

// #include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

#ifndef NDEBUG
#include <job_logger.h>
#endif

namespace job::ggml {

JobGgmlDeviceManager::JobGgmlDeviceManager(bool autoScan)
{
    if (autoScan)
        scan();
}

void JobGgmlDeviceManager::scan()
{
    if (m_state == DeviceManagerState::Ready)
        return;

    reset();
    setState(DeviceManagerState::Scanning);

    try {
        static const bool backendsLoaded = []() {
            ggml_backend_load_all();
            return true;
        }();
        (void)backendsLoaded;

        const std::size_t nativeDeviceCount = ggml_backend_dev_count();

        if (nativeDeviceCount == 0)
            throw std::runtime_error{ "GGML did not report any registered backend devices" };

        m_devices.reserve(nativeDeviceCount);

        for (std::size_t index = 0; index < nativeDeviceCount; ++index) {
            ggml_backend_dev_t nativeDevice = ggml_backend_dev_get(index);

            if (!nativeDevice)
                continue;

            JobGgmlBackendReg::Ptr backendReg;
            const ggml_backend_reg_t nativeReg = ggml_backend_dev_backend_reg(nativeDevice);
            if (nativeReg) {
                backendReg = registryFromNative(nativeReg);

                if (!backendReg) {
                    backendReg = JobGgmlBackendReg::createShared(nativeReg);
                    m_backendRegistries.append(backendReg);
                }
            }

            const JobGgmlDeviceImpl impl = backendReg ? deviceImplFromName(backendReg->name()) : JobGgmlDeviceImpl::Fallback;
            JobGgmlDevice::Ptr wrappedDevice;

            switch (impl) {
            case JobGgmlDeviceImpl::Cpu:
                wrappedDevice = JobGgmlCpu::createShared(nativeDevice, std::move(backendReg));
                break;

            case JobGgmlDeviceImpl::Fallback:
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
                break;

            case JobGgmlDeviceImpl::Vulkan:
#ifdef JOB_GGML_VULKAN
                wrappedDevice = JobGgmlVulkan::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::Cuda:
#ifdef JOB_GGML_CUDA
                wrappedDevice = JobGgmlCuda::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::OpenCl:
#ifdef JOB_GGML_OPENCL
                wrappedDevice = JobGgmlOpenCl::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::Blas:
#ifdef JOB_GGML_BLAS
                wrappedDevice = JobGgmlBlas::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::Hexagon:
#ifdef JOB_GGML_HEXAGON
                wrappedDevice = JobGgmlHexagon::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::OpenVino:
#ifdef JOB_GGML_OPENVINO
                wrappedDevice = JobGgmlOpenVino::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::Sycl:
#ifdef JOB_GGML_SYCL
                wrappedDevice = JobGgmlSycl::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::WebGpu:
#ifdef JOB_GGML_WEBGPU
                wrappedDevice = JobGgmlWebGpu::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::Zdnn:
#ifdef JOB_GGML_ZDNN
                wrappedDevice =
                    JobGgmlZdnn::createShared(nativeDevice, std::move(backendReg));
#else
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
#endif
                break;

            case JobGgmlDeviceImpl::VirtGpu:
            case JobGgmlDeviceImpl::Metal:
            case JobGgmlDeviceImpl::ZenDnn:
            case JobGgmlDeviceImpl::Cann:
            case JobGgmlDeviceImpl::Rpc:
            case JobGgmlDeviceImpl::Count:
#ifndef NDEBUG
                JOB_LOG_DEBUG(
                    "Device implementation has not yet been added; "
                    "falling back to JobGgmlDevice. Device name: {}",
                    backendReg ? backendReg->name() : "unknown"
                    );
#endif
                wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));
                break;
            }

            if (!wrappedDevice || !wrappedDevice->isValid())
                throw std::runtime_error{ "Failed to construct a valid JobGgmlDevice" };

            const std::string uid = wrappedDevice->uid();

            if (uid.empty() || uid == "unknown")
                throw std::runtime_error{ "GGML device did not provide a usable uid" };

            m_devices.insert(wrappedDevice);
            indexDevice(uid, impl, wrappedDevice);
        }

        if (m_devices.isEmpty())
            throw std::runtime_error{ "GGML devices were registered, but none could be wrapped" };

        if (!m_cpuDevice)
            throw std::runtime_error{ "GGML did not report a usable CPU device" };

        setState(DeviceManagerState::Ready);
    } catch (const std::exception &error) {
        reset();

        setErrorString(error.what());
        setState(DeviceManagerState::Error);
    } catch (...) {
        reset();

        setErrorString("Unknown error while scanning GGML devices");
        setState(DeviceManagerState::Error);
    }
}

DeviceManagerState JobGgmlDeviceManager::state() const noexcept
{
    return m_state;
}

bool JobGgmlDeviceManager::isReady() const noexcept
{
    return m_state == DeviceManagerState::Ready;
}

bool JobGgmlDeviceManager::isValid() const noexcept
{
    return isReady() &&
           !m_devices.isEmpty() &&
           m_cpuDevice &&
           m_cpuDevice->isValid();
}

const std::string &JobGgmlDeviceManager::errorString() const noexcept
{
    return m_errorString;
}

std::size_t JobGgmlDeviceManager::deviceCount() const noexcept
{
    return m_devices.size();
}

std::size_t JobGgmlDeviceManager::gpuDeviceCount() const noexcept
{
    std::size_t ret = 0;
    for (auto i = m_devices.begin(); i != m_devices.end(); ++i)
        if (i->second->props()->deviceType() == JobGgmlDeviceType::IGpu || i->second->props()->deviceType() == JobGgmlDeviceType::Gpu)
            ++ret;
    return ret;
}

const JobGgmlDeviceManager::Devices &JobGgmlDeviceManager::devices() const noexcept
{
    return m_devices; // This sucks and should be owneed ... *
}

JobGgmlDevice *JobGgmlDeviceManager::device(const std::string &uid)
{
    if (uid.empty() || !m_devices.contains(uid))
        return nullptr;

    return m_devices.at(uid);
}

JobGgmlDevice *JobGgmlDeviceManager::device(std::size_t idx)
{
    if(idx >= m_devices.size())
        return nullptr;
    return m_devices.at(idx);
}

JobGgmlCpu *JobGgmlDeviceManager::cpu()
{
    if (!m_cpuDevice || !m_cpuDevice->isValid())
        return nullptr;
    return m_cpuDevice.get();
}

#ifdef JOB_GGML_VULKAN
JobGgmlDeviceManager::VulkanDevices JobGgmlDeviceManager::vulkanDevices()
{
    return m_vulkanDevices;
}

JobGgmlVulkan *JobGgmlDeviceManager::vulkan(const std::string &uid)
{
    if(uid.empty() || !m_vulkanDevices.contains(uid))
        return nullptr;
    return m_vulkanDevices.at(uid);
}

JobGgmlVulkan *JobGgmlDeviceManager::vulkan(std::size_t idx)
{
    if (idx >= m_vulkanDevices.size())
        return nullptr;
    return m_vulkanDevices.at(idx);
}

bool JobGgmlDeviceManager::hasVulkan() const noexcept
{
    return !m_vulkanDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_CUDA
JobGgmlDeviceManager::CudaDevices JobGgmlDeviceManager::cudaDevices()
{
    return m_cudaDevices;
}

JobGgmlCuda *JobGgmlDeviceManager::cuda(const std::string &uid)
{
    if (uid.empty() || !m_cudaDevices.contains(uid))
        return nullptr;

    return m_cudaDevices.at(uid);
}

JobGgmlCuda *JobGgmlDeviceManager::cuda(std::size_t idx)
{
    if(idx >= m_cudaDevices.size())
        return nullptr;
    return m_cudaDevices.at(idx);
}

bool JobGgmlDeviceManager::hasCuda() const noexcept
{
    return !m_cudaDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_OPENCL
JobGgmlDeviceManager::OpenClDevices JobGgmlDeviceManager::openClDevices()
{
    return m_openClDevices;
}

JobGgmlOpenCl *JobGgmlDeviceManager::openCl(const std::string &uid)
{
    if (uid.empty() || !m_openClDevices.contains(uid))
        return nullptr;

    return m_openClDevices.at(uid);
}
JobGgmlOpenCl *JobGgmlDeviceManager::openCl(std::size_t idx)
{
    if (idx >= m_openClDevices.size())
        return nullptr;

    return m_openClDevices.at(idx);
}

bool JobGgmlDeviceManager::hasOpenCl() const noexcept
{
    return !m_openClDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_BLAS
JobGgmlDeviceManager::BlasDevices JobGgmlDeviceManager::blasDevices()
{
    return m_blasDevices;
}

JobGgmlBlas *JobGgmlDeviceManager::blas(const std::string &uid)
{
    if(uid.empty() || !m_blasDevices.contains(uid))
        return nullptr;

    return m_blasDevices.at(uid);
}

JobGgmlBlas *JobGgmlDeviceManager::blas(std::size_t idx)
{
    if(idx >= m_blasDevices.count())
        return nullptr;
    return m_blasDevices.at(idx);
}

bool JobGgmlDeviceManager::hasBlas() const noexcept
{
    return !m_blasDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_HEXAGON
JobGgmlDeviceManager::HexagonDevices JobGgmlDeviceManager::hexagonDevices()
{
    return m_hexagonDevices;
}

JobGgmlHexagon *JobGgmlDeviceManager::hexagon(const std::string &uid)
{
    if(uid.empty() || !m_hexagonDevices.contains(uid))
        return nullptr;
    return m_hexagonDevices.at(uid);
}

JobGgmlHexagon *JobGgmlDeviceManager::hexagon(std::size_t idx)
{
    if (idx >= m_hexagonDevices.size())
        return nullptr;
    return m_hexagonDevices.at(idx);
}

bool JobGgmlDeviceManager::hasHexagon() const noexcept
{
    return !m_hexagonDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_OPENVINO
JobGgmlDeviceManager::OpenVinoDevices JobGgmlDeviceManager::openVinoDevices()
{
    return m_openVinoDevices;
}

JobGgmlOpenVino *JobGgmlDeviceManager::openVino(const std::string &uid)
{
    if (uid.empty() || !m_openVinoDevices.contains(uid))
        return nullptr;
    return m_openVinoDevices.at(uid);
}

JobGgmlOpenVino *JobGgmlDeviceManager::openVino(std::size_t idx)
{
    if (idx >= m_openVinoDevices.size())
        return nullptr;
    return m_openVinoDevices.at(idx);
}

bool JobGgmlDeviceManager::hasOpenVino() const noexcept
{
    return !m_openVinoDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_SYCL
JobGgmlDeviceManager::SyclDevices JobGgmlDeviceManager::syclDevices()
{
    return m_syclDevices;
}

JobGgmlSycl *JobGgmlDeviceManager::sycl(const std::string &uid)
{
    if (uid.empty() || !m_syclDevices.contains(uid))
        return nullptr;
    return m_syclDevices.at(uid);
}

JobGgmlSycl *JobGgmlDeviceManager::sycl(std::size_t idx)
{
    if (idx >= m_syclDevices.size())
        return nullptr;
    return m_syclDevices.at(idx);
}

bool JobGgmlDeviceManager::hasSycl() const noexcept
{
    return !m_syclDevices.isEmpty();
}
#endif

#ifdef JOB_GGML_WEBGPU
JobGgmlDeviceManager::WebGpuDevices JobGgmlDeviceManager::webGpuDevices()
{
    return m_webGpuDevices;
}

JobGgmlWebGpu *JobGgmlDeviceManager::webGpu(const std::string &uid)
{
    if (uid.empty() || !m_webGpuDevices.contains(uid))
        return nullptr;

    return m_webGpuDevices.at(uid);
}

JobGgmlWebGpu *JobGgmlDeviceManager::webGpu(std::size_t idx)
{
    if (idx >= m_webGpuDevices.size())
        return nullptr;

    return m_webGpuDevices.at(idx);
}

bool JobGgmlDeviceManager::hasWebGpu() const noexcept
{
    return !m_webGpuDevices.isEmpty();
}

#endif

#ifdef JOB_GGML_ZDNN
JobGgmlDeviceManager::ZdnnDevices &JobGgmlDeviceManager::zdnnDevices()
{
    return m_zdnnDevices;
}

JobGgmlZdnn *JobGgmlDeviceManager::zdnn(const std::string &uid)
{
    if (uid.empty() || !m_zdnnDevices.contains(uid))
        return nullptr;

    return m_zdnnDevices.at(uid);
}

JobGgmlZdnn *JobGgmlDeviceManager::zdnn(std::size_t idx)
{
    if (idx >= m_zdnnDevices.size())
        return nullptr;

    return m_zdnnDevices.at(idx);
}

bool JobGgmlDeviceManager::hasZdnn() const noexcept
{
    return !m_zdnnDevices.isEmpty();
}
#endif

JobGgmlDeviceManager::GpuDevices JobGgmlDeviceManager::fallbackGpus()
{
    return m_fallbackGpus;
}

bool JobGgmlDeviceManager::hasCpu() const noexcept
{
    return m_cpuDevice && m_cpuDevice->isValid();
}

bool JobGgmlDeviceManager::hasGpu() const noexcept
{
    if (!m_fallbackGpus.isEmpty())
        return true;

#ifdef JOB_GGML_VULKAN
    if (!m_vulkanDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_CUDA
    if (!m_cudaDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_OPENCL
    if (!m_openClDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_HEXAGON
    if (!m_hexagonDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_OPENVINO
    if (!m_openVinoDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_SYCL
    if (!m_syclDevices.isEmpty())
        return true;
#endif

#ifdef JOB_GGML_WEBGPU
    if (!m_webGpuDevices.isEmpty())
        return true;
#endif

    return false;
}


std::vector<JobGgmlBackendSched::Ptr> JobGgmlDeviceManager::buildScheduler(const std::vector<std::string> &uids,
                                                                           const std::vector<std::size_t> &graphSizes,
                                                                           const std::vector<bool> &parallelFlags,
                                                                           const std::vector<bool> &opOffloadFlags)
{
    const std::size_t count = uids.size();

    if (graphSizes.size() != count ||
        parallelFlags.size() != count ||
        opOffloadFlags.size() != count) {
        throw std::invalid_argument{
            "buildScheduler requires equally sized scheduler parameter lists"
        };
    }
    std::vector<JobGgmlBackendSched::Ptr> schedulers;
    schedulers.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        schedulers.push_back(buildScheduler(uids[index],
                                            graphSizes[index],
                                            parallelFlags[index],
                                            opOffloadFlags[index]));
    }

    return schedulers;
}


JobGgmlBackendSched::Ptr JobGgmlDeviceManager::buildScheduler(const std::string &uid,
                                                              std::size_t graphSize,
                                                              bool parallel,
                                                              bool opOffload)
{
    if (!isReady())
        throw std::runtime_error{ "Cannot build a scheduler before the device manager is ready" };

    if (uid.empty())
        throw std::invalid_argument{ "buildScheduler requires a device uid" };

    if (graphSize == 0)
        throw std::invalid_argument{ "buildScheduler requires a graph size greater than zero" };

    JobGgmlDevice *dev = device(uid);
    if (!dev || !dev->isValid())
        throw std::invalid_argument{ "buildScheduler requires a valid device owned by this manager" };

    JobGgmlBackend::Ptr backend = dev->backend();

    if (!backend || !backend->isValid())
        throw std::runtime_error{ "Requested device does not expose a valid backend" };

    // core::JobList<JobGgmlBackend::Ptr> backends;
    JobGgmlBackendSched::Backends  backends;
    backends.push_back(backend);

    if (dev != m_cpuDevice.get()) {
        if (!m_cpuDevice || !m_cpuDevice->isValid())
            throw std::runtime_error{ "A CPU device is required as the scheduler fallback" };

        JobGgmlBackend::Ptr cpuBackend = m_cpuDevice->backend();

        if (!cpuBackend || !cpuBackend->isValid())
            throw std::runtime_error{ "CPU device does not expose a valid backend" };

        if (cpuBackend->backend() != backend->backend())
            backends.push_back(std::move(cpuBackend));
    }

    JobGgmlBackendSched::BufferTypes bufferTypes;
    auto scheduler = JobGgmlBackendSched::createShared(
        std::move(backends),
        std::move(bufferTypes),
        graphSize,
        parallel,
        opOffload
        );

    if (!scheduler || !scheduler->isValid())
        throw std::runtime_error{ "Failed to create a valid GGML backend scheduler" };

    m_scheduler = scheduler;
    return m_scheduler;
}

JobGgmlBackendSched::Ptr JobGgmlDeviceManager::buildScheduler(JobGgmlDevice *dev, std::size_t graphSize, bool parallel, bool opOffload)
{
    if (!dev)
        throw std::invalid_argument{ "buildScheduler requires a valid device" };

    return buildScheduler(dev->uid(), graphSize, parallel, opOffload);
}

JobGgmlBackendSched::Ptr JobGgmlDeviceManager::scheduler() const noexcept
{
    return m_scheduler;
}

bool JobGgmlDeviceManager::hasScheduler() const noexcept
{
    return m_scheduler && m_scheduler->isValid();
}

void JobGgmlDeviceManager::resetScheduler() noexcept
{
    m_scheduler.reset();
}

void JobGgmlDeviceManager::reset() noexcept
{
    resetScheduler();
    m_cpuDevice.reset();
    m_fallbackGpus.clear();

#ifdef JOB_GGML_VULKAN
    m_vulkanDevices.clear();
#endif

#ifdef JOB_GGML_CUDA
    m_cudaDevices.clear();
#endif

#ifdef JOB_GGML_OPENCL
    m_openClDevices.clear();
#endif

#ifdef JOB_GGML_BLAS
    m_blasDevices.clear();
#endif

#ifdef JOB_GGML_HEXAGON
    m_hexagonDevices.clear();
#endif

#ifdef JOB_GGML_OPENVINO
    m_openVinoDevices.clear();
#endif

#ifdef JOB_GGML_SYCL
    m_syclDevices.clear();
#endif

#ifdef JOB_GGML_WEBGPU
    m_webGpuDevices.clear();
#endif

#ifdef JOB_GGML_ZDNN
    m_zdnnDevices.clear();
#endif

    // Canonical owner last.
    m_devices.clear();
    m_backendRegistries.clear();

    clearError();
    setState(DeviceManagerState::Uninitialized);
}

std::string JobGgmlDeviceManager::debugString() const
{
    std::ostringstream stream;

    stream
        << "JobGgmlDeviceManager{"
        << "state=";

    switch (m_state) {
    case DeviceManagerState::Uninitialized:
        stream << "Uninitialized";
        break;

    case DeviceManagerState::Scanning:
        stream << "Scanning";
        break;

    case DeviceManagerState::Ready:
        stream << "Ready";
        break;

    case DeviceManagerState::Error:
        stream << "Error";
        break;
    }

    stream
        << ", devices=" << m_devices.size()
        << ", gpus=" << gpuDeviceCount()
        << ", fallbackGpus=" << m_fallbackGpus.size()
        << ", hasCpu=" << (hasCpu() ? "true" : "false")
        << ", hasScheduler="
        << (hasScheduler() ? "true" : "false");

    if (!m_errorString.empty()) {
        stream
            << ", error=\""
            << m_errorString
            << '"';
    }

    stream << '}';

    return stream.str();
}

void JobGgmlDeviceManager::setState(DeviceManagerState state) noexcept
{
    m_state = state;
}

void JobGgmlDeviceManager::setErrorString(const std::string &errorString)
{
    m_errorString = errorString;
}

void JobGgmlDeviceManager::clearError() noexcept
{
    m_errorString.clear();
}

JobGgmlBackendReg::Ptr JobGgmlDeviceManager::registryFromNative(ggml_backend_reg_t backendReg) const noexcept
{
    if (!backendReg)
        return nullptr;

    for (const JobGgmlBackendReg::Ptr &candidate : m_backendRegistries) {
        if (candidate && candidate->backendReg() == backendReg)
            return candidate;
    }

    return nullptr;
}

void JobGgmlDeviceManager::indexDevice(const std::string &uid,
                                       JobGgmlDeviceImpl impl,
                                       const JobGgmlDevice::Ptr &device)
{
    if (uid.empty() || uid == "unknown")
        throw std::invalid_argument{ "Cannot index a JobGgmlDevice without a usable uid" };

    if (!device || !device->isValid())
        throw std::invalid_argument{ "Cannot index an invalid JobGgmlDevice" };

    const JobGgmlDeviceType deviceType = device->props()->deviceType();
    switch (impl) {
    case JobGgmlDeviceImpl::Cpu:
    {
        JobGgmlCpu::Ptr cpu = std::dynamic_pointer_cast<JobGgmlCpu>(device);
        if (!cpu)
            throw std::runtime_error{ "GGML CPU device was not wrapped as JobGgmlCpu" };

        if (!m_cpuDevice)
            m_cpuDevice = std::move(cpu);

        break;
    }

    case JobGgmlDeviceImpl::Vulkan:
#ifdef JOB_GGML_VULKAN
    {
        if (m_vulkanDevices.contains(uid))
            break;
        JobGgmlVulkan *vulkan = dynamic_cast<JobGgmlVulkan *>(device.get());
        if (!vulkan)
            throw std::runtime_error{ "GGML Vulkan device was not wrapped as JobGgmlVulkan" };
        m_vulkanDevices.insert(uid, vulkan);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Cuda:
#ifdef JOB_GGML_CUDA
    {
        if (m_cudaDevices.contains(uid))
            break;
        JobGgmlCuda *cuda = dynamic_cast<JobGgmlCuda *>(device.get());

        if (!cuda)
            throw std::runtime_error{ "GGML CUDA device was not wrapped as JobGgmlCuda" };
        m_cudaDevices.insert(uid, cuda);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::OpenCl:
#ifdef JOB_GGML_OPENCL
    {
        if (m_openClDevices.contains(uid))
            break;
        JobGgmlOpenCl *openCl = dynamic_cast<JobGgmlOpenCl *>(device.get());
        if (!openCl)
            throw std::runtime_error{ "GGML OpenCL device was not wrapped as JobGgmlOpenCl" };
        m_openClDevices.insert(uid, openCl);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Blas:
#ifdef JOB_GGML_BLAS
    {
        if (m_blasDevices.contains(uid))
            break;
        JobGgmlBlas *blas = dynamic_cast<JobGgmlBlas *>(device.get());
        if (!blas)
            throw std::runtime_error{ "GGML BLAS device was not wrapped as JobGgmlBlas" };
        m_blasDevices.insert(uid, blas);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Hexagon:
#ifdef JOB_GGML_HEXAGON
    {
        if (m_hexagonDevices.contains(uid))
            break;
        JobGgmlHexagon *hexagon = dynamic_cast<JobGgmlHexagon *>(device.get());
        if (!hexagon)
            throw std::runtime_error{ "GGML Hexagon device was not wrapped as JobGgmlHexagon" };
        m_hexagonDevices.insert(uid, hexagon);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::OpenVino:
#ifdef JOB_GGML_OPENVINO
    {
        if (m_openVinoDevices.contains(uid))
            break;

        JobGgmlOpenVino *openVino = dynamic_cast<JobGgmlOpenVino *>(device.get());
        if (!openVino)
            throw std::runtime_error{ "GGML OpenVINO device was not wrapped as JobGgmlOpenVino" };
        m_openVinoDevices.insert(uid, openVino);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Sycl:
#ifdef JOB_GGML_SYCL
    {
        if (m_syclDevices.contains(uid))
            break;
        JobGgmlSycl *sycl = dynamic_cast<JobGgmlSycl *>(device.get());
        if (!sycl)
            throw std::runtime_error{ "GGML SYCL device was not wrapped as JobGgmlSycl" };
        m_syclDevices.insert(uid, sycl);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::WebGpu:
#ifdef JOB_GGML_WEBGPU
    {
        if (m_webGpuDevices.contains(uid))
            break;
        JobGgmlWebGpu *webGpu = dynamic_cast<JobGgmlWebGpu *>(device.get());
        if (!webGpu)
            throw std::runtime_error{ "GGML WebGPU device was not wrapped as JobGgmlWebGpu" };
        m_webGpuDevices.insert(uid, webGpu);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Zdnn:
#ifdef JOB_GGML_ZDNN
    {
        if (m_zdnnDevices.contains(uid))
            break;
        JobGgmlZdnn *zdnn = dynamic_cast<JobGgmlZdnn *>(device.get());
        if (!zdnn)
            throw std::runtime_error{ "GGML zDNN device was not wrapped as JobGgmlZdnn" };
        m_zdnnDevices.insert(uid, zdnn);
        break;
    }
#else
        break;
#endif

    case JobGgmlDeviceImpl::Fallback:
    case JobGgmlDeviceImpl::VirtGpu:
    case JobGgmlDeviceImpl::Metal:
    case JobGgmlDeviceImpl::ZenDnn:
    case JobGgmlDeviceImpl::Cann:
    case JobGgmlDeviceImpl::Rpc:
    case JobGgmlDeviceImpl::Count:
        if ((deviceType == JobGgmlDeviceType::Gpu || deviceType == JobGgmlDeviceType::IGpu) && !m_fallbackGpus.contains(uid)){
            m_fallbackGpus.insert(uid, device.get());
        }
        break;
    }
}

} // namespace job::ggml