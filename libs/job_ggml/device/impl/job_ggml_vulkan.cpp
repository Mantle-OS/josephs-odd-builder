#include "job_ggml_vulkan.h"

#include <sstream>

namespace job::ggml {

JobGgmlVulkan::JobGgmlVulkan(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{

}

bool JobGgmlVulkan::isVulkanBackend() const noexcept
{
    const auto nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_vk(nativeBackend->backend());
}

int JobGgmlVulkan::deviceCount() noexcept
{
    return ggml_backend_vk_get_device_count();
}

std::string JobGgmlVulkan::dump()
{
    std::ostringstream stream;
    stream
        << "Vulkan{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isVulkanBackend() ? "true" : "false");

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