#include "job_ggml_hexagon.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlHexagon::JobGgmlHexagon(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{

}

bool JobGgmlHexagon::isHexagonBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();
    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_hexagon(nativeBackend->backend());
}

std::string JobGgmlHexagon::dump()
{
    std::ostringstream stream;
    stream
        << "Hexagon{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isHexagonBackend() ? "true" : "false");

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