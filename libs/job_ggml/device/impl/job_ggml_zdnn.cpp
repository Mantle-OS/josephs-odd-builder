#include "job_ggml_zdnn.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlZdnn::JobGgmlZdnn(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{ device, std::move(backendReg) }
{

}

std::string JobGgmlZdnn::dump()
{
    std::ostringstream stream;

    stream
        << "zDNN{"
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