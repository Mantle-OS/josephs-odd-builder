#include "job_ggml_sycl.h"

#include <array>
#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlSycl::JobGgmlSycl(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{
}

bool JobGgmlSycl::isSyclBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_sycl(nativeBackend->backend());
}

int JobGgmlSycl::deviceCount() noexcept
{
    return ggml_backend_sycl_get_device_count();
}

std::vector<int> JobGgmlSycl::gpuList()
{
    const int count = deviceCount();
    if (count <= 0)
        return {};

    std::vector<int> devices(static_cast<std::size_t>(count), -1);
    ggml_backend_sycl_get_gpu_list(devices.data(), count);
    return devices;
}

void JobGgmlSycl::printDevices()
{
    ggml_backend_sycl_print_sycl_devices();
}

std::string JobGgmlSycl::deviceDescription(int device)
{
    if (device < 0 || device >= deviceCount())
        return {};

    std::array<char, 1024> description{};
    ggml_backend_sycl_get_device_description(device, description.data(), description.size());

    return std::string{description.data()};
}

std::size_t JobGgmlSycl::deviceFreeMemory(int device) noexcept
{
    if (device < 0 || device >= deviceCount())
        return 0;

    std::size_t freeMemory  = 0;
    std::size_t totalMemory = 0;

    ggml_backend_sycl_get_device_memory(device, &freeMemory, &totalMemory);

    return freeMemory;
}

std::size_t JobGgmlSycl::deviceTotalMemory(int device) noexcept
{
    if (device < 0 || device >= deviceCount())
        return 0;

    std::size_t freeMemory  = 0;
    std::size_t totalMemory = 0;
    ggml_backend_sycl_get_device_memory(device, &freeMemory, &totalMemory );

    return totalMemory;
}

JobGgmlBackendBufferType::Ptr JobGgmlSycl::splitBufferType(std::span<const float> tensorSplit)
{
    if (tensorSplit.empty())
        return nullptr;

    ggml_backend_buffer_type_t rawBufferType = ggml_backend_sycl_split_buffer_type(tensorSplit.data());

    if (!rawBufferType)
        return nullptr;

    return JobGgmlBackendBufferType::createShared(rawBufferType);
}

std::string JobGgmlSycl::dump()
{
    std::ostringstream stream;

    stream
        << "SYCL{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isSyclBackend() ? "true" : "false");

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