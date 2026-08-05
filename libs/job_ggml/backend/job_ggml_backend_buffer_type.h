#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "job_ggml_backend_buffer.h"
#include "job_ggml_tensor.h"

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBackendBufferType
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendBufferType>;
    using UPtr = std::unique_ptr<JobGgmlBackendBufferType>;

    explicit JobGgmlBackendBufferType(ggml_backend_buffer_type_t bufferType);
    ~JobGgmlBackendBufferType() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_buffer_type_t bufferType) { return std::make_shared<JobGgmlBackendBufferType>(bufferType); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_buffer_type_t bufferType) { return std::make_unique<JobGgmlBackendBufferType>(bufferType); }

    JobGgmlBackendBufferType(const JobGgmlBackendBufferType &) = delete;
    JobGgmlBackendBufferType &operator=(const JobGgmlBackendBufferType &) = delete;
    JobGgmlBackendBufferType(JobGgmlBackendBufferType &&) = delete;
    JobGgmlBackendBufferType &operator=(JobGgmlBackendBufferType &&) = delete;

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] std::size_t alignment() const noexcept;
    void setAlignment(std::size_t alignment) noexcept;

    [[nodiscard]] std::size_t maxSize() const noexcept;
    void setMaxSize(std::size_t maxSize) noexcept;

    [[nodiscard]] bool isHost() const noexcept;
    void setIsHost(bool isHost) noexcept;

    // [[nodiscard]] ggml_backend_buffer_ptr allocateBuffer(std::size_t size) const;
    [[nodiscard]] std::unique_ptr<JobGgmlBackendBuffer> allocateBuffer(std::size_t size) const;
    [[nodiscard]] std::size_t allocationSize(const ggml_tensor *tensor) const noexcept;
    [[nodiscard]] std::size_t allocationSize(const JobGgmlTensor &tensor) const noexcept;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] ggml_backend_dev_t device() const noexcept;
    [[nodiscard]] ggml_backend_buffer_type_t bufferType() const noexcept;


private:
    ggml_backend_buffer_type_t m_bufferType{nullptr}; // Borrowed from GGML.
    ggml_backend_dev_t         m_device{nullptr};     // Borrowed association.

    std::string                m_name{"unknown"};
    std::size_t                m_alignment{0};
    std::size_t                m_maxSize{0};
    bool                       m_isHost{false};
};

} // namespace job::ggml