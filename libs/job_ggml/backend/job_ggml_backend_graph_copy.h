#pragma once

#include <memory>

#include <ggml-backend.h>
#include <ggml.h>

#include "job_ggml_backend.h"
#include "job_ggml_backend_buffer_view.h"
#include "job_ggml_cgraph.h"
#include "job_ggml_context_view.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBackendGraphCopy
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendGraphCopy>;
    using WPtr = std::weak_ptr<JobGgmlBackendGraphCopy>;
    using UPtr = std::unique_ptr<JobGgmlBackendGraphCopy>;

    explicit JobGgmlBackendGraphCopy(JobGgmlBackend &backend, JobGgmlCGraph &graph);
    ~JobGgmlBackendGraphCopy();

    [[nodiscard]] static Ptr createShared(JobGgmlBackend &backend, JobGgmlCGraph &graph)
    {
        return std::make_shared<JobGgmlBackendGraphCopy>(backend, graph);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlBackend &backend, JobGgmlCGraph &graph)
    {
        return std::make_unique<JobGgmlBackendGraphCopy>(backend, graph);
    }

    JobGgmlBackendGraphCopy(const JobGgmlBackendGraphCopy &) = delete;
    JobGgmlBackendGraphCopy &operator=(const JobGgmlBackendGraphCopy &) = delete;
    JobGgmlBackendGraphCopy(JobGgmlBackendGraphCopy &&) = delete;
    JobGgmlBackendGraphCopy &operator=(JobGgmlBackendGraphCopy &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    /*
     * Borrowed object views.
     * These wrappers never free their native objects. The native aggregate stored in m_graphCopy remains the sole owner and must outlive every returned view.
     */
    [[nodiscard]] JobGgmlBackendBufferView::UPtr bufferObject() const;
    [[nodiscard]] JobGgmlContextView::UPtr allocatedContextObject() const;
    [[nodiscard]] JobGgmlContextView::UPtr unallocatedContextObject() const;
    [[nodiscard]] JobGgmlCGraph::UPtr graphObject() const;

    [[nodiscard]] ggml_backend_buffer_t buffer() const noexcept;

    [[nodiscard]] ggml_context *allocatedContext() noexcept;
    [[nodiscard]] const ggml_context * allocatedContext() const noexcept;

    [[nodiscard]] ggml_context *unallocatedContext() noexcept;
    [[nodiscard]] const ggml_context *
    unallocatedContext() const noexcept;

    [[nodiscard]] ggml_cgraph *graph() noexcept;
    [[nodiscard]] const ggml_cgraph *graph() const noexcept;

    [[nodiscard]] const struct ggml_backend_graph_copy &graphCopy() const noexcept;

    void reset() noexcept;

private:
    struct ggml_backend_graph_copy m_graphCopy{}; // Owned as one aggregate and released only through ggml_backend_graph_copy_free(). Its native members must never be placed into independently owning JOB wrappers.
};

} // namespace job::ggml