#pragma once

#include <memory>

#include <ggml-backend.h>

#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlBackend;

class JOBGGML_EXPORT JobGgmlBackendGraphPlan
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendGraphPlan>;
    using WPtr = std::weak_ptr<JobGgmlBackendGraphPlan>;
    using UPtr = std::unique_ptr<JobGgmlBackendGraphPlan>;

    /*
     * Takes ownership of the supplied native graph plan.
     *
     * The backend is borrowed and MUST outlive this object because GGML
     * requires the originating backend when releasing a graph plan.
     */
    explicit JobGgmlBackendGraphPlan(JobGgmlBackend *backend, ggml_backend_graph_plan_t plan);

    ~JobGgmlBackendGraphPlan();

    [[nodiscard]] static Ptr createShared(JobGgmlBackend *backend, ggml_backend_graph_plan_t plan)
    {
        return std::make_shared<JobGgmlBackendGraphPlan>(backend, plan);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlBackend *backend, ggml_backend_graph_plan_t plan)
    {
        return std::make_unique<JobGgmlBackendGraphPlan>(backend, plan);
    }

    JobGgmlBackendGraphPlan(const JobGgmlBackendGraphPlan &) = delete;
    JobGgmlBackendGraphPlan &operator=(const JobGgmlBackendGraphPlan &) = delete;
    JobGgmlBackendGraphPlan(JobGgmlBackendGraphPlan &&) = delete;
    JobGgmlBackendGraphPlan &operator=(JobGgmlBackendGraphPlan &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    void reset() noexcept;

    [[nodiscard]] ggml_backend_graph_plan_t plan() const noexcept;

    [[nodiscard]] JobGgmlBackend *backend() noexcept;
    [[nodiscard]] const JobGgmlBackend *backend() const noexcept;

private:
    JobGgmlBackend              *m_backend{nullptr};    // Borrowed; must outlive the plan.
    ggml_backend_graph_plan_t   m_plan{nullptr};        // Owned.
};

} // namespace job::ggml