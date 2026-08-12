#include "job_ggml_opencl.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlOpenCl::JobGgmlOpenCl(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{

}


bool JobGgmlOpenCl::isOpenClBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_opencl(nativeBackend->backend());
}


std::string JobGgmlOpenCl::dump()
{
    std::ostringstream stream;

    stream
        << "OpenCL{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isOpenClBackend() ? "true" : "false");

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