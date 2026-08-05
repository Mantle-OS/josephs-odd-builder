#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "job_ggml_backend_event.h"
#include "job_ggml_cgraph.h"
#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBackend
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackend>;
    using WPtr = std::weak_ptr<JobGgmlBackend>;
    using UPtr = std::unique_ptr<JobGgmlBackend>;

    explicit JobGgmlBackend(ggml_backend_t backend); // Takes ownership of the supplied native backend.
    explicit JobGgmlBackend(ggml_backend_ptr backend); // Takes ownership by moving the native RAII backend.

    ~JobGgmlBackend() = default;

    [[nodiscard]] static Ptr createShared(ggml_backend_t backend)
    {
        return std::make_shared<JobGgmlBackend>(backend);
    }

    [[nodiscard]] static Ptr createShared(ggml_backend_ptr backend)
    {
        return std::make_shared<JobGgmlBackend>(std::move(backend));
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_t backend)
    {
        return std::make_unique<JobGgmlBackend>(backend);
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_ptr backend)
    {
        return std::make_unique<JobGgmlBackend>(std::move(backend));
    }

    JobGgmlBackend(const JobGgmlBackend &) = delete;
    JobGgmlBackend &operator=(const JobGgmlBackend &) = delete;
    JobGgmlBackend(JobGgmlBackend &&) = delete;
    JobGgmlBackend &operator=(JobGgmlBackend &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isCpu() const noexcept;

    [[nodiscard]] const std::string &name() const noexcept;
    void setName(const std::string &name);

    [[nodiscard]] ggml_backend_t backend() const noexcept;

    /* BACKPORT
     * Replace the native device accessor with a JobGgmlDevice association
     * when the device subsystem ownership model is finalized.
     */
    [[nodiscard]] ggml_backend_dev_t device() const noexcept;

    void setTensorAsync(
        JobGgmlTensor &tensor,
        const void *data,
        std::size_t offset,
        std::size_t size
        );

    void getTensorAsync(
        const JobGgmlTensor &tensor,
        void *data,
        std::size_t offset,
        std::size_t size
        );

    void setTensor2dAsync(
        JobGgmlTensor &tensor,
        const void *data,
        std::size_t offset,
        std::size_t size,
        std::size_t copies,
        std::size_t tensorStride,
        std::size_t dataStride
        );

    void getTensor2dAsync(
        const JobGgmlTensor &tensor,
        void *data,
        std::size_t offset,
        std::size_t size,
        std::size_t copies,
        std::size_t tensorStride,
        std::size_t dataStride
        );

    void copyTensorAsync(JobGgmlBackend &destination, const JobGgmlTensor &source, JobGgmlTensor &target);

    void synchronize();

    /* BACKPORT
     * Graph plans do not yet have a JOB wrapper.
     * Replace ggml_backend_graph_plan_t when that object is introduced.
     */
    [[nodiscard]] ggml_backend_graph_plan_t createGraphPlan(JobGgmlCGraph &graph);

    /* BACKPORT
     * Graph plans do not yet have a JOB wrapper.
     * Replace ggml_backend_graph_plan_t when that object is introduced.
     */
    void freeGraphPlan(ggml_backend_graph_plan_t plan) noexcept;

    /* BACKPORT
     * Graph plans do not yet have a JOB wrapper.
     * Replace ggml_backend_graph_plan_t when that object is introduced.
     */
    [[nodiscard]] JobGgmlStatus computeGraphPlan(ggml_backend_graph_plan_t plan);

    [[nodiscard]] JobGgmlStatus computeGraph(JobGgmlCGraph &graph);

    [[nodiscard]] JobGgmlStatus computeGraphAsync(JobGgmlCGraph &graph);

    void recordEvent(JobGgmlBackendEvent &event);
    void waitEvent(JobGgmlBackendEvent &event);

    [[nodiscard]] static constexpr JobGgmlStatus fromGgmlStatus(enum ggml_status status) noexcept
    {
        return static_cast<JobGgmlStatus>(status);
    }

    [[nodiscard]] static constexpr enum ggml_status toGgmlStatus(JobGgmlStatus status) noexcept
    {
        return static_cast<enum ggml_status>(status);
    }

private:
    ggml_backend_ptr m_backend;
    std::string      m_name{"unknown"};
};

} // namespace job::ggml