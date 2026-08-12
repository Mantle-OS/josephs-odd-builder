#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "job_ggml_backend_event.h"
#include "job_ggml_backend_graph_plan.h"

#include "job_ggml_cgraph.h"

#include "job_ggml_enums.h"

#include "job_ggml_tensor.h"

#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlDevice ;
class JOBGGML_EXPORT JobGgmlBackend
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackend>;
    using WPtr = std::weak_ptr<JobGgmlBackend>;
    using UPtr = std::unique_ptr<JobGgmlBackend>;

    explicit JobGgmlBackend(ggml_backend_t backend);    // Takes ownership of the supplied native backend.
    explicit JobGgmlBackend(ggml_backend_ptr backend);  // Takes ownership by moving the native RAII backend.

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

    [[nodiscard]] ggml_backend_dev_t nativeDevice() const noexcept;
    [[nodiscard]] JobGgmlDevice *device() noexcept;
    [[nodiscard]] const JobGgmlDevice *device() const noexcept;

    void setTensorAsync(JobGgmlTensor &tensor,
                        const void *data,
                        std::size_t offset,
                        std::size_t size);

    void getTensorAsync(const JobGgmlTensor &tensor,
                        void *data,
                        std::size_t offset,
                        std::size_t size);

    void setTensor2dAsync(JobGgmlTensor &tensor,
                          const void *data,
                          std::size_t offset,
                          std::size_t size,
                          std::size_t copies,
                          std::size_t tensorStride,
                          std::size_t dataStride);

    void getTensor2dAsync(const JobGgmlTensor &tensor,
                          void *data,
                          std::size_t offset,
                          std::size_t size,
                          std::size_t copies,
                          std::size_t tensorStride,
                          std::size_t dataStride);

    void copyTensorAsync(JobGgmlBackend &destination, const JobGgmlTensor &source, JobGgmlTensor &target);
    void synchronize();

    [[nodiscard]] JobGgmlBackendGraphPlan::UPtr createGraphPlan(JobGgmlCGraph &graph);
    [[nodiscard]] JobGgmlStatus computeGraphPlan(JobGgmlBackendGraphPlan &plan);
    [[nodiscard]] JobGgmlStatus computeGraph(JobGgmlCGraph &graph);
    [[nodiscard]] JobGgmlStatus computeGraphAsync(JobGgmlCGraph &graph);

    void recordEvent(JobGgmlBackendEvent &event);
    void waitEvent(JobGgmlBackendEvent &event);

    // wishfull thinking #include "job_ggml_abort_callback.h"
    // void JobGgmlBackend::setAbortCallback(JobGgmlAbortCallback *callback)
    // {
    //     if (!m_backend)
    //         throw std::runtime_error{ "Cannot set an abort callback on an invalid GGML backend" };
    //     ggml_backend_set_abort_callback(
    //         m_backend.get(),
    //         callback ? callback->callback() : nullptr,
    //         callback ? callback->callbackData() : nullptr
    //         );
    // }

private:
    ggml_backend_ptr    m_backend;
    JobGgmlDevice       *m_device{nullptr}; // Borrowed.
    std::string         m_name{"unknown"};
};

} // namespace job::ggml