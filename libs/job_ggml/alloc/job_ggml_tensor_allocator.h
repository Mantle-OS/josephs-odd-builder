#pragma once

#include <cstddef>
#include <memory>

#include <ggml-alloc.h>

#include "job_ggml_backend_buffer.h"
#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorAllocator
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorAllocator>;
    using WPtr = std::weak_ptr<JobGgmlTensorAllocator>;
    using UPtr = std::unique_ptr<JobGgmlTensorAllocator>;

    explicit JobGgmlTensorAllocator(JobGgmlBackendBuffer::Ptr buffer);
    explicit JobGgmlTensorAllocator(JobGgmlBackendBuffer::Ptr buffer, const struct ggml_tallocr &tensorAllocator);

    ~JobGgmlTensorAllocator() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlBackendBuffer::Ptr buffer) { return std::make_shared<JobGgmlTensorAllocator>(std::move(buffer)); }
    [[nodiscard]] static Ptr createShared(JobGgmlBackendBuffer::Ptr buffer, const struct ggml_tallocr &tensorAllocator) { return std::make_shared<JobGgmlTensorAllocator>(std::move(buffer), tensorAllocator); }

    [[nodiscard]] static UPtr createUniq(JobGgmlBackendBuffer::Ptr buffer) { return std::make_unique<JobGgmlTensorAllocator>(std::move(buffer)); }
    [[nodiscard]] static UPtr createUniq(JobGgmlBackendBuffer::Ptr buffer, const struct ggml_tallocr &tensorAllocator) { return std::make_unique<JobGgmlTensorAllocator>(std::move(buffer), tensorAllocator); }

    JobGgmlTensorAllocator(const JobGgmlTensorAllocator &) = delete;
    JobGgmlTensorAllocator &operator=(const JobGgmlTensorAllocator &) = delete;
    JobGgmlTensorAllocator(JobGgmlTensorAllocator &&) = delete;
    JobGgmlTensorAllocator &operator=(JobGgmlTensorAllocator &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] JobGgmlBackendBuffer::Ptr buffer() const noexcept;
    [[nodiscard]] void *base() noexcept;
    [[nodiscard]] const void *base() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;
    [[nodiscard]] std::size_t offset() const noexcept;
    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::size_t remaining() const noexcept;

    [[nodiscard]] JobGgmlStatus allocate(JobGgmlTensor &tensor) noexcept;
    [[nodiscard]] JobGgmlStatus allocate(struct ggml_tensor *tensor) noexcept;
    [[nodiscard]] static std::size_t requiredBufferSize(const JobGgmlBackendBufferType &bufferType, const JobGgmlTensor &tensor) noexcept;

    void setTensorAllocator(const struct ggml_tallocr &other) noexcept;
    [[nodiscard]] struct ggml_tallocr tensorAllocator() noexcept;
    void resetTensorAllocator() noexcept;


private:
    [[nodiscard]] static constexpr JobGgmlStatus fromGgmlStatus(enum ggml_status status) noexcept { return static_cast<JobGgmlStatus>(status); }
    JobGgmlBackendBuffer::Ptr m_buffer; // Shared ownership keeps the native buffer alive while this allocator uses it.
    struct ggml_tallocr       m_tensorAllocator{};
    void                     *m_base{nullptr};  // Borrowed from m_buffer.
    std::size_t               m_alignment{0};
    std::size_t               m_offset{0};
};

} // namespace job::ggml