#pragma once


#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <ggml-backend.h>
#include <ggml.h>

#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

/*
 * JobGgmlBackendBufferView exists for native buffers whose lifetime is
 * controlled by another GGML object.
 *
 * The immediate use case is JobGgmlBackendGraphCopy. Its native
 * ggml_backend_graph_copy aggregate owns a buffer together with two contexts
 * and a graph, and releases all of them through
 * ggml_backend_graph_copy_free().
 *
 * Wrapping that buffer in JobGgmlBackendBuffer would be incorrect because
 * JobGgmlBackendBuffer owns its ggml_backend_buffer_t and would attempt to
 * free it independently. This class provides the same inspection vocabulary
 * while borrowing the native buffer and never releasing it.
 *
 * The owner of the native buffer must outlive this view.
 */

namespace job::ggml {
class JOBGGML_EXPORT JobGgmlBackendBufferView
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendBufferView>;
    using WPtr = std::weak_ptr<JobGgmlBackendBufferView>;
    using UPtr = std::unique_ptr<JobGgmlBackendBufferView>;

    /*
     * Borrows the supplied native buffer.
     *
     * The buffer is never freed by this object. Its native owner must outlive
     * the JobGgmlBackendBufferView.
     */
    explicit JobGgmlBackendBufferView(
        ggml_backend_buffer_t buffer
        );

    ~JobGgmlBackendBufferView() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_buffer_t buffer)
    {
        return std::make_shared<JobGgmlBackendBufferView>(buffer);
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_buffer_t buffer)
    {
        return std::make_unique<JobGgmlBackendBufferView>(buffer);
    }

    JobGgmlBackendBufferView(const JobGgmlBackendBufferView &) = delete;

    JobGgmlBackendBufferView &operator=(const JobGgmlBackendBufferView &) = delete;

    JobGgmlBackendBufferView(JobGgmlBackendBufferView &&) = delete;
    JobGgmlBackendBufferView &operator=(JobGgmlBackendBufferView &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] ggml_backend_buffer_t buffer() const noexcept;
    [[nodiscard]] ggml_backend_buffer_type_t bufferType() const noexcept;

    [[nodiscard]] bool isHost() const noexcept;

    [[nodiscard]] void *base() noexcept;
    [[nodiscard]] const void *base() const noexcept;

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;
    [[nodiscard]] std::size_t maxSize() const noexcept;

    [[nodiscard]] std::size_t allocationSize(const JobGgmlTensor &tensor) const noexcept;

    [[nodiscard]] std::size_t allocationSize(const ggml_tensor *tensor) const noexcept;
    [[nodiscard]] JobGgmlBackendBufferUsage usage() const noexcept;

private:
    ggml_backend_buffer_t m_buffer{nullptr}; // Borrowed; never freed by this object.
    std::string           m_name{"unknown"};
};

} // namespace job::ggml