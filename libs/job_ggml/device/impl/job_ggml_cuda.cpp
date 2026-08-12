#include "job_ggml_cuda.h"

#include <charconv>
#include <sstream>
#include <string_view>
#include <vector>

#include <cuda_runtime.h>

namespace job::ggml {

namespace {

class CudaDeviceGuard
{
public:
    explicit CudaDeviceGuard(int newDevice) noexcept
    {
        if (cudaGetDevice(&m_previousDevice) != cudaSuccess)
            return;

        if (m_previousDevice == newDevice) {
            m_valid = true;
            return;
        }

        if (cudaSetDevice(newDevice) != cudaSuccess)
            return;

        m_changed = true;
        m_valid   = true;
    }

    ~CudaDeviceGuard()
    {
        if (m_valid && m_changed)
            (void)cudaSetDevice(m_previousDevice);
    }

    CudaDeviceGuard(const CudaDeviceGuard &) = delete;
    CudaDeviceGuard &operator=(const CudaDeviceGuard &) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_valid;
    }

private:
    int  m_previousDevice{-1};
    bool m_changed{false};
    bool m_valid{false};
};

} // namespace


JobGgmlCuda::JobGgmlCuda(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{
}


bool JobGgmlCuda::isCudaBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_cuda(nativeBackend->backend());
}


int JobGgmlCuda::deviceCount() noexcept
{
    return ggml_backend_cuda_get_device_count();
}

int JobGgmlCuda::cudaDeviceIndex() const noexcept
{
    if (!device())
        return -1;

    const char *nativeName = ggml_backend_dev_name(device());

    if (!nativeName)
        return -1;

    const std::string_view name{ nativeName };
    // GGML CUDA devices are exposed as:
    //
    //     CUDA0
    //     CUDA1
    //     ...
    //
    // Do not infer the CUDA ordinal from discovery order. The native device name already contains the ordinal assigned by the backend.


    constexpr std::string_view Prefix{ GGML_CUDA_NAME };
    if (!name.starts_with(Prefix))
        return -1;

    const std::string_view ordinal = name.substr(Prefix.size());
    if (ordinal.empty())
        return -1;

    int index = -1;
    const auto result = std::from_chars(ordinal.data(),
                                        ordinal.data() + ordinal.size(),
                                        index
                                        );

    if (result.ec != std::errc{} || result.ptr != ordinal.data() + ordinal.size() || index < 0)
        return -1;

    return index;
}


bool JobGgmlCuda::canAccessPeer(const JobGgmlCuda &targetDevice) const noexcept
{
    const int srcIdx = cudaDeviceIndex();
    const int dstIdx = targetDevice.cudaDeviceIndex();

    if (srcIdx < 0 || dstIdx < 0 || srcIdx == dstIdx)
        return false;

    int canAccess = 0;
    const cudaError_t status = cudaDeviceCanAccessPeer(&canAccess, srcIdx, dstIdx);
    return status == cudaSuccess && canAccess != 0;
}


bool JobGgmlCuda::enablePeerAccess(JobGgmlCuda &targetDevice) noexcept
{
    const int srcIdx = cudaDeviceIndex();
    const int dstIdx = targetDevice.cudaDeviceIndex();

    if (srcIdx < 0 || dstIdx < 0 || srcIdx == dstIdx)
        return false;

    if (!canAccessPeer(targetDevice))
        return false;

    CudaDeviceGuard guard{ srcIdx };
    if (!guard.isValid())
        return false;

    const cudaError_t status = cudaDeviceEnablePeerAccess(dstIdx, 0);
    if (status == cudaErrorPeerAccessAlreadyEnabled) {
        // Clear CUDA's per-thread last error state.
        (void)cudaGetLastError();
        return true;
    }

    return status == cudaSuccess;
}

bool JobGgmlCuda::enableBiDirectionalP2P(JobGgmlCuda &dev0, JobGgmlCuda &dev1) noexcept
{
    if (!dev0.canAccessPeer(dev1) || !dev1.canAccessPeer(dev0))
        return false;

    if (!dev0.enablePeerAccess(dev1))
        return false;

    return dev1.enablePeerAccess(dev0);
}

bool JobGgmlCuda::copyToPeer(void *dstPtr,
                             const JobGgmlCuda &dstDevice,
                             const void *srcPtr,
                             std::size_t sizeBytes) const noexcept
{
    const int srcIdx = cudaDeviceIndex();
    const int dstIdx = dstDevice.cudaDeviceIndex();

    if (!dstPtr ||
        !srcPtr ||
        sizeBytes == 0 ||
        srcIdx < 0 ||
        dstIdx < 0 ||
        srcIdx == dstIdx) {
        return false;
    }

    if (!canAccessPeer(dstDevice))
        return false;

    CudaDeviceGuard guard{ srcIdx };
    if (!guard.isValid())
        return false;

    return cudaMemcpyPeer(dstPtr,
                          dstIdx,
                          srcPtr,
                          srcIdx,
                          sizeBytes
                          ) == cudaSuccess;
}


bool JobGgmlCuda::registerHostBuffer(void *buffer, std::size_t size) noexcept
{
    if (!buffer || size == 0)
        return false;

    return ggml_backend_cuda_register_host_buffer(buffer, size);
}


void JobGgmlCuda::unregisterHostBuffer(void *buffer) noexcept
{
    if (!buffer)
        return;

    ggml_backend_cuda_unregister_host_buffer(buffer);
}

JobGgmlBackendBufferType::Ptr JobGgmlCuda::splitBufferType(int mainDevice, std::span<const float> tensorSplit)
{
    if (mainDevice < 0 || tensorSplit.empty())
        return nullptr;    
    ggml_backend_buffer_type_t rawBufferType = ggml_backend_cuda_split_buffer_type(mainDevice, tensorSplit.data());
    if (!rawBufferType)
        return nullptr;

    return JobGgmlBackendBufferType::createShared(rawBufferType);
}


bool JobGgmlCuda::allReduceTensor(std::span<const JobGgmlBackend::Ptr> backends,
                                  std::span<const JobGgmlTensor::Ptr> tensors)
{
    if (backends.empty() || tensors.empty() || backends.size() != tensors.size())
        return false;


    std::vector<ggml_backend_t> nativeBackends;
    std::vector<ggml_tensor *> nativeTensors;

    nativeBackends.reserve(backends.size());
    nativeTensors.reserve(tensors.size());

    for (std::size_t index = 0; index < backends.size(); ++index) {
        const JobGgmlBackend::Ptr &backend = backends[index];
        const JobGgmlTensor::Ptr &tensor = tensors[index];
        if (!backend ||
            !backend->isValid() ||
            !tensor ||
            !tensor->isValid()) {
            return false;
        }

        nativeBackends.push_back(backend->backend());
        nativeTensors.push_back(tensor->tensor());
    }

    return ggml_backend_cuda_allreduce_tensor(nativeBackends.data(),
                                              nativeTensors.data(),
                                              nativeBackends.size()
                                              );
}

std::string JobGgmlCuda::dump()
{
    std::ostringstream stream;

    stream
        << "CUDA{"
        << "index=" << cudaDeviceIndex()
        << ", valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isCudaBackend() ? "true" : "false");

    if (props()) {
        stream
            << ", name=" << props()->name()
            << ", description=" << props()->description()
            << ", freeMemory=" << props()->memoryFree()
            << ", totalMemory=" << props()->memoryTotal();
    }

    stream << '}';

    return stream.str();
}

} // namespace job::ggml