#pragma once
#include <memory>
#include <string>

#include <ggml-backend.h>

namespace job::ggml {
class JobGgmlDevice {
public:
    using Ptr = std::shared_ptr<JobGgmlDevice>;
    using UPtr = std::unique_ptr<JobGgmlDevice>;

    static UPtr mkUniq(ggml_backend_dev_t dev){
        return std::make_unique<JobGgmlDevice>(dev);
    }

    JobGgmlDevice(ggml_backend_dev_t dev);
    ~JobGgmlDevice();

    [[nodiscard]] const std::string &name() const noexcept;
    [[nodiscard]] enum ggml_backend_dev_type type() const noexcept;

    [[nodiscard]] size_t memoryFree() const noexcept;
    [[nodiscard]] size_t memoryTotal() const noexcept;

    [[nodiscard]] const ggml_backend_dev_props &props() const noexcept;

    [[nodiscard]] ggml_backend_t backend() const noexcept;

    [[nodiscard]] ggml_backend_buffer_type_t bufferType() const noexcept;
    [[nodiscard]] ggml_backend_buffer_type_t hostBufferType() const noexcept;

    [[nodiscard]] bool hasEvents() const noexcept;

    JobGgmlDevice(const JobGgmlDevice &) = delete;
    JobGgmlDevice &operator=(const JobGgmlDevice &) = delete;

private:
    // Only JobGgmlDeviceManager can construct

    friend class JobGgmlDeviceManager;

    ggml_backend_dev_t           m_dev;
    ggml_backend_dev_props       m_props{};
    ggml_backend_t               m_backend{nullptr};
    ggml_backend_buffer_type_t   m_bufferType{nullptr};
    ggml_backend_buffer_type_t   m_hostBufferType{nullptr};
    std::string                  m_name;
};
}
