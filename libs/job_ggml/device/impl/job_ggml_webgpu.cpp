#include "job_ggml_webgpu.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlWebGpu::JobGgmlWebGpu(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{

}

std::string JobGgmlWebGpu::dump()
{
    std::ostringstream stream;

    stream
        << "WebGPU{"
        << "valid=" << (isValid() ? "true" : "false");

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