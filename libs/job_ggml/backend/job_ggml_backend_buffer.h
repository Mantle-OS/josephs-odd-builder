#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlBackendBufferType;

class JOBGGML_EXPORT JobGgmlBackendBuffer
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendBuffer>;
    using WPtr = std::weak_ptr<JobGgmlBackendBuffer>;
    using UPtr = std::unique_ptr<JobGgmlBackendBuffer>;

    // Takes ownership of the supplied native buffer.
    explicit JobGgmlBackendBuffer(ggml_backend_buffer_t buffer);
    // Takes ownership by moving the native RAII buffer.
    explicit JobGgmlBackendBuffer(ggml_backend_buffer_ptr buffer);
    ~JobGgmlBackendBuffer() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_buffer_t buffer) { return std::make_shared<JobGgmlBackendBuffer>(buffer); }
    [[nodiscard]] static Ptr createShared(ggml_backend_buffer_ptr buffer) { return std::make_shared<JobGgmlBackendBuffer>(std::move(buffer)); }

    [[nodiscard]] static UPtr createUniq(ggml_backend_buffer_t buffer) { return std::make_unique<JobGgmlBackendBuffer>(buffer); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_buffer_ptr buffer) { return std::make_unique<JobGgmlBackendBuffer>(std::move(buffer)); }

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] ggml_backend_buffer_t buffer() const noexcept;
    [[nodiscard]] ggml_backend_buffer_type_t bufferType() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isHost() const noexcept;

    [[nodiscard]] void *base() noexcept;
    [[nodiscard]] const void *base() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;
    [[nodiscard]] std::size_t maxSize() const noexcept;

    [[nodiscard]] std::size_t allocationSize(const ggml_tensor *tensor) const noexcept;
    [[nodiscard]] std::size_t allocationSize(const JobGgmlTensor &tensor) const noexcept;

    [[nodiscard]] JobGgmlStatus initializeTensor(JobGgmlTensor &tensor);
    [[nodiscard]] JobGgmlStatus initializeTensor(ggml_tensor *tensor);

    [[nodiscard]] JobGgmlBackendBufferUsage usage() const noexcept;
    void setUsage(JobGgmlBackendBufferUsage usage) noexcept;

    void clear(std::uint8_t value = 0) noexcept;
    void reset() noexcept;

    [[nodiscard]] static constexpr JobGgmlBackendBufferUsage fromGgmlBufferUsage(enum ggml_backend_buffer_usage usage) noexcept
    {
        return static_cast<JobGgmlBackendBufferUsage>(usage);
    }

    [[nodiscard]] static constexpr enum ggml_backend_buffer_usage toGgmlBufferUsage(JobGgmlBackendBufferUsage usage) noexcept
    {
        return static_cast<enum ggml_backend_buffer_usage>(usage);
    }

    JobGgmlBackendBuffer(const JobGgmlBackendBuffer &) = delete;
    JobGgmlBackendBuffer &operator=(const JobGgmlBackendBuffer &) = delete;
    JobGgmlBackendBuffer(JobGgmlBackendBuffer &&) = delete;
    JobGgmlBackendBuffer &operator=(JobGgmlBackendBuffer &&) = delete;

private:
    ggml_backend_buffer_ptr m_buffer;
    std::string             m_name{"unknown"};
};

} // namespace job::ggml