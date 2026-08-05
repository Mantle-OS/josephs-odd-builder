#pragma once

#include <cstdint>
#include <memory>

#include <ggml.h>

#include "job_ggml_tensor_rank.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorBatch final : public BatchRank
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorBatch>;
    using UPtr = std::unique_ptr<JobGgmlTensorBatch>;

    explicit JobGgmlTensorBatch(struct ggml_tensor *tensor);
    ~JobGgmlTensorBatch() override = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor)
    { return std::make_shared<JobGgmlTensorBatch>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorBatch>( tensor ); }

    JobGgmlTensorBatch(const JobGgmlTensorBatch &) = delete;
    JobGgmlTensorBatch &operator=(const JobGgmlTensorBatch &) = delete;
    JobGgmlTensorBatch(JobGgmlTensorBatch &&) = delete;
    JobGgmlTensorBatch &operator=(JobGgmlTensorBatch &&) = delete;

    [[nodiscard]] std::int64_t width() const noexcept;
    [[nodiscard]] std::int64_t height() const noexcept;
    [[nodiscard]] std::int64_t depth() const noexcept;
    [[nodiscard]] std::int64_t batchCount() const noexcept;

    [[nodiscard]] std::int64_t columns() const noexcept;
    [[nodiscard]] std::int64_t rows() const noexcept;
    [[nodiscard]] std::int64_t planeCount() const noexcept;

    [[nodiscard]] std::size_t elementStride() const noexcept;
    [[nodiscard]] std::size_t rowStride() const noexcept;
    [[nodiscard]] std::size_t planeStride() const noexcept;
    [[nodiscard]] std::size_t batchStride() const noexcept;

    [[nodiscard]] std::int64_t elementsPerPlane() const noexcept;
    [[nodiscard]] std::int64_t elementsPerBatch() const noexcept;

    [[nodiscard]] bool isSingleBatch() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
};

} // namespace job::ggml