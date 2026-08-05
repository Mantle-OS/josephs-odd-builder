#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <ggml.h>
#include <ggml-cpp.h>

#include "job_ggml_cgraph.h"
#include "job_ggml_enums.h"
#include "job_ggml_init_params.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlContext
{
public:
    using Ptr  = std::shared_ptr<JobGgmlContext>;
    using WPtr = std::weak_ptr<JobGgmlContext>;
    using UPtr = std::unique_ptr<JobGgmlContext>;

    explicit JobGgmlContext(const JobGgmlInitParams &initParams);
    explicit JobGgmlContext(const ggml_init_params &initParams);
    explicit JobGgmlContext(ggml_context_ptr context);

    ~JobGgmlContext() = default;

    [[nodiscard]] static Ptr createShared(const JobGgmlInitParams &initParams){ return std::make_shared<JobGgmlContext>(initParams); }

    [[nodiscard]] static Ptr createShared(const ggml_init_params &initParams) { return std::make_shared<JobGgmlContext>(initParams); }
    [[nodiscard]] static Ptr createShared(ggml_context_ptr context) { return std::make_shared<JobGgmlContext>(std::move(context)); }

    [[nodiscard]] static UPtr createUniq(const JobGgmlInitParams &initParams){ return std::make_unique<JobGgmlContext>(initParams);}
    [[nodiscard]] static UPtr createUniq(const ggml_init_params &initParams) { return std::make_unique<JobGgmlContext>(initParams); }
    [[nodiscard]] static UPtr createUniq(ggml_context_ptr context) { return std::make_unique<JobGgmlContext>(std::move(context)); }

    JobGgmlContext(const JobGgmlContext &) = delete;
    JobGgmlContext &operator=(const JobGgmlContext &) = delete;
    JobGgmlContext(JobGgmlContext &&) = delete;
    JobGgmlContext &operator=(JobGgmlContext &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] ggml_context *context() noexcept;
    [[nodiscard]] const ggml_context *context() const noexcept;

    void reset() noexcept;

    [[nodiscard]] std::size_t usedMemory() const noexcept;
    [[nodiscard]] std::size_t memorySize() const noexcept;
    [[nodiscard]] std::size_t maxTensorSize() const noexcept;

    [[nodiscard]] void *memoryBuffer() noexcept;
    [[nodiscard]] const void *memoryBuffer() const noexcept;

    [[nodiscard]] bool noAlloc() const noexcept;
    void setNoAlloc(bool noAlloc) noexcept;
    [[nodiscard]] void *newBuffer(std::size_t size);

    // Tensor creation
    [[nodiscard]] JobGgmlTensor::UPtr newTensor( JobGgmlType type, int dimensions, const std::int64_t *extents);
    [[nodiscard]] JobGgmlTensor::UPtr newTensor1d(JobGgmlType type, std::int64_t ne0);
    [[nodiscard]] JobGgmlTensor::UPtr newTensor2d(JobGgmlType type, std::int64_t ne0, std::int64_t ne1);
    [[nodiscard]] JobGgmlTensor::UPtr newTensor3d(JobGgmlType type, std::int64_t ne0, std::int64_t ne1, std::int64_t ne2);
    [[nodiscard]] JobGgmlTensor::UPtr newTensor4d(JobGgmlType type, std::int64_t ne0, std::int64_t ne1, std::int64_t ne2, std::int64_t ne3);
    [[nodiscard]] JobGgmlTensor::UPtr duplicateTensor(const JobGgmlTensor &source);
    [[nodiscard]] JobGgmlTensor::UPtr viewTensor(JobGgmlTensor &source);

    // Tensor iteration and lookup
    [[nodiscard]] JobGgmlTensor::UPtr firstTensor();
    [[nodiscard]] JobGgmlTensor::UPtr nextTensor(const JobGgmlTensor &tensor);
    [[nodiscard]] JobGgmlTensor::UPtr tensor(const std::string &name);

    // Graph creation
    [[nodiscard]] JobGgmlCGraph::UPtr newGraph();
    [[nodiscard]] JobGgmlCGraph::UPtr newGraphCustom(std::size_t size, bool gradients);
    [[nodiscard]] JobGgmlCGraph::UPtr duplicateGraph(JobGgmlCGraph &graph, bool forceGradients);

private:
    [[nodiscard]] static constexpr enum ggml_type toGgmlType(JobGgmlType type) noexcept { return static_cast<enum ggml_type>(type); }

    ggml_context_ptr m_context; // Owned by this object.
};

} // namespace job::ggml