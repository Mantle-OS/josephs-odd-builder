#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <functional>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "job_ggml_backend.h"
#include "job_ggml_backend_buffer_type.h"
#include "job_ggml_cgraph.h"
#include "jobggml_export.h"
namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBackendSched
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendSched>;
    using WPtr = std::weak_ptr<JobGgmlBackendSched>;
    using UPtr = std::unique_ptr<JobGgmlBackendSched>;
    //  the call back for the qt bindings.
    using EvalCallback   = std::function<bool(JobGgmlTensor &tensor, bool ask)>;
    using BufferTypes    = std::vector<JobGgmlBackendBufferType::Ptr>;
    using Backends       = std::vector<JobGgmlBackend::Ptr>;

    explicit JobGgmlBackendSched(Backends backends,
                                 BufferTypes bufferTypes = {},
                                 std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                 bool parallel = true,
                                 bool opOffload = true);

    ~JobGgmlBackendSched() = default;

    [[nodiscard]] static Ptr createShared(Backends backends,
                                          BufferTypes bufferTypes = {},
                                          std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                          bool parallel = true,
                                          bool opOffload = true)
    {
        return std::make_shared<JobGgmlBackendSched>(std::move(backends),
                                                     std::move(bufferTypes),
                                                     graphSize,
                                                     parallel,
                                                     opOffload);
    }

    [[nodiscard]] static UPtr createUniq(Backends backends,
                                         BufferTypes bufferTypes = {},
                                         std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                         bool parallel = true,
                                         bool opOffload = true)
    {
        return std::make_unique<JobGgmlBackendSched>(std::move(backends),
                                                     std::move(bufferTypes),
                                                     graphSize,
                                                     parallel,
                                                     opOffload);
    }

    JobGgmlBackendSched(const JobGgmlBackendSched &) = delete;
    JobGgmlBackendSched &operator=(const JobGgmlBackendSched &) = delete;
    JobGgmlBackendSched(JobGgmlBackendSched &&) = delete;
    JobGgmlBackendSched &operator=(JobGgmlBackendSched &&) = delete;

    [[nodiscard]] ggml_backend_sched_t scheduler() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] std::size_t graphSize() const noexcept;
    [[nodiscard]] bool parallel() const noexcept;
    [[nodiscard]] bool opOffload() const noexcept;

    [[nodiscard]] int backendCount() const noexcept;
    [[nodiscard]] JobGgmlBackend::Ptr backend(int index) const noexcept;

    [[nodiscard]] int splitCount() const noexcept;
    [[nodiscard]] int copyCount() const noexcept;

    [[nodiscard]] JobGgmlBackendBufferType::Ptr bufferType(const JobGgmlBackend &backend) const noexcept;
    [[nodiscard]] std::size_t bufferSize(const JobGgmlBackend &backend) const noexcept;

    void reserveSize(JobGgmlCGraph &measureGraph, std::vector<std::size_t> &sizes);
    [[nodiscard]] bool reserve(JobGgmlCGraph &measureGraph);

    void setTensorBackend(JobGgmlTensor &tensor, const JobGgmlBackend &backend);
    [[nodiscard]] JobGgmlBackend::Ptr tensorBackend(const JobGgmlTensor &tensor) const noexcept;

    void splitGraph(JobGgmlCGraph &graph);

    [[nodiscard]] bool allocateGraph(JobGgmlCGraph &graph);

    [[nodiscard]] JobGgmlStatus computeGraph(JobGgmlCGraph &graph);
    [[nodiscard]] JobGgmlStatus computeGraphAsync(JobGgmlCGraph &graph);

    void synchronize();
    void reset();

    void setEvalCallback(EvalCallback callback);
    void clearEvalCallback() noexcept;

private:
    [[nodiscard]] static bool evalCallbackTrampoline(ggml_tensor *tensor, bool ask, void *userData) noexcept;
    [[nodiscard]] JobGgmlBackend::Ptr backendFromNative(ggml_backend_t backend) const noexcept;
    [[nodiscard]] JobGgmlBackendBufferType::Ptr bufferTypeFromNative(ggml_backend_buffer_type_t bufferType) const noexcept;

    EvalCallback            m_evalCallback;
    Backends                m_backends;
    BufferTypes             m_bufferTypes;
    ggml_backend_sched_ptr  m_scheduler;
    std::size_t             m_graphSize{GGML_DEFAULT_GRAPH_SIZE};
    bool                    m_parallel{true};
    bool                    m_opOffload{true};
};

} // namespace job::ggml