#pragma once

#include <cstdint>
#include <memory>

#include <ggml.h>

#include "job_ggml_tensor_rank.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorMatrix final : public MatrixRank
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorMatrix>;
    using UPtr = std::unique_ptr<JobGgmlTensorMatrix>;

    explicit JobGgmlTensorMatrix(struct ggml_tensor *tensor);
    ~JobGgmlTensorMatrix() override = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorMatrix>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorMatrix>(tensor); }

    JobGgmlTensorMatrix(const JobGgmlTensorMatrix &) = delete;

    JobGgmlTensorMatrix &operator=(const JobGgmlTensorMatrix &) = delete;
    JobGgmlTensorMatrix(JobGgmlTensorMatrix &&) = delete;
    JobGgmlTensorMatrix &operator=(JobGgmlTensorMatrix &&) = delete;

    [[nodiscard]] std::int64_t rows() const noexcept;
    [[nodiscard]] std::int64_t columns() const noexcept;

    [[nodiscard]] std::size_t elementStride() const noexcept;
    [[nodiscard]] std::size_t rowStride() const noexcept;

    [[nodiscard]] bool isSquare() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
};

} // namespace job::ggml