#pragma once

/*
 * JobGgmlContextView provides non-owning access to a native ggml_context.
 *
 * The immediate use case is JobGgmlBackendGraphCopy. Its native
 * ggml_backend_graph_copy aggregate owns two contexts:
 *
 *     ctx_allocated
 *     ctx_unallocated
 *
 * Those contexts are released together with the aggregate through
 * ggml_backend_graph_copy_free(). Wrapping either pointer in JobGgmlContext
 * would create a second owner and cause an invalid or double free.
 *
 * This class never releases the native context. The native owner must outlive
 * every JobGgmlContextView and every tensor or graph wrapper obtained from it.
 */

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <ggml.h>

#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlContextView
{
public:
    using Ptr  = std::shared_ptr<JobGgmlContextView>;
    using WPtr = std::weak_ptr<JobGgmlContextView>;
    using UPtr = std::unique_ptr<JobGgmlContextView>;
    using CustomPayloads = std::vector<std::shared_ptr<void>>;

    /*
     * Borrows the supplied native context. The context is never freed by this object.
     */
    explicit JobGgmlContextView(ggml_context *context);

    ~JobGgmlContextView() = default;

    [[nodiscard]] static Ptr createShared(ggml_context *context)
    {
        return std::make_shared<JobGgmlContextView>(context);
    }

    [[nodiscard]] static UPtr createUniq(ggml_context *context)
    {
        return std::make_unique<JobGgmlContextView>(context);
    }

    JobGgmlContextView(const JobGgmlContextView &) = delete;
    JobGgmlContextView &operator=(const JobGgmlContextView &) = delete;
    JobGgmlContextView(JobGgmlContextView &&) = delete;
    JobGgmlContextView &operator=(JobGgmlContextView &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] ggml_context *context() noexcept;
    [[nodiscard]] const ggml_context *context() const noexcept;

    [[nodiscard]] std::size_t usedMemory() const noexcept;
    [[nodiscard]] std::size_t memorySize() const noexcept;
    [[nodiscard]] std::size_t maxTensorSize() const noexcept;

    [[nodiscard]] void *memoryBuffer() noexcept;
    [[nodiscard]] const void *memoryBuffer() const noexcept;

    [[nodiscard]] bool noAlloc() const noexcept;

    // Tensor iteration and lookup
    [[nodiscard]] JobGgmlTensor::UPtr firstTensor();

    [[nodiscard]] JobGgmlTensor::UPtr nextTensor(const JobGgmlTensor &tensor);

    [[nodiscard]] JobGgmlTensor::UPtr tensor(const std::string &name);

private:
    ggml_context    *m_context{nullptr}; // Borrowed; never freed by this object.
};

} // namespace job::ggml