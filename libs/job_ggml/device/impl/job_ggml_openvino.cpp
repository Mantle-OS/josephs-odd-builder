#include "job_ggml_openvino.h"

#include <sstream>
#include <utility>

namespace job::ggml {

JobGgmlOpenVino::JobGgmlOpenVino(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice{device, std::move(backendReg)}
{
}

bool JobGgmlOpenVino::isOpenVinoBackend() const noexcept
{
    const JobGgmlBackend::Ptr nativeBackend = backend();

    return nativeBackend && nativeBackend->isValid() && ggml_backend_is_openvino(nativeBackend->backend());
}

int JobGgmlOpenVino::deviceCount() noexcept
{
    return ggml_backend_openvino_get_device_count();
}

bool JobGgmlOpenVino::isOpenVinoBuffer(const JobGgmlBackendBuffer &buffer) noexcept
{
    if (!buffer.isValid())
        return false;

    return ggml_backend_buffer_is_openvino(buffer.buffer());
}

bool JobGgmlOpenVino::isOpenVinoBufferType(const JobGgmlBackendBufferType &bufferType) noexcept
{
    if (!bufferType.isValid())
        return false;

    return ggml_backend_buft_is_openvino(bufferType.bufferType());
}

bool JobGgmlOpenVino::isOpenVinoHostBufferType(const JobGgmlBackendBufferType &bufferType) noexcept
{
    if (!bufferType.isValid())
        return false;

    return ggml_backend_buft_is_openvino_host(bufferType.bufferType());
}

std::optional<std::size_t> JobGgmlOpenVino::bufferContextId(const JobGgmlBackendBuffer &buffer) noexcept
{
    if (!buffer.isValid())
        return std::nullopt;

    if (!isOpenVinoBuffer(buffer))
        return std::nullopt;

    return ggml_backend_openvino_buffer_get_ctx_id(buffer.buffer());
}

std::string JobGgmlOpenVino::dump()
{
    std::ostringstream stream;
    stream
        << "OpenVINO{"
        << "valid=" << (isValid() ? "true" : "false")
        << ", backend=" << (isOpenVinoBackend() ? "true" : "false");

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