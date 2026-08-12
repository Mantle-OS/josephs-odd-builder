#include "job_ggml_blas.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlBlas::JobGgmlBlas(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{
}


bool JobGgmlBlas::isBlasBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_blas(nativeBackend->backend());
}

std::string JobGgmlBlas::dump()
{
    std::ostringstream stream;
    stream
        << "BLAS{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isBlasBackend() ? "true" : "false");

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