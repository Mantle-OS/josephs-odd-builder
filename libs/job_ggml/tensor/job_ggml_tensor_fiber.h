#pragma once

#include <cstdint>
#include <memory>

#include <ggml.h>

#include "job_ggml_tensor_rank.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorFiber final : public FiberRank
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorFiber>;
    using UPtr = std::unique_ptr<JobGgmlTensorFiber>;

    explicit JobGgmlTensorFiber(struct ggml_tensor *tensor);
    ~JobGgmlTensorFiber() override = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor){ return std::make_shared<JobGgmlTensorFiber>(tensor);}

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor){ return std::make_unique<JobGgmlTensorFiber>(tensor);}

    JobGgmlTensorFiber( const JobGgmlTensorFiber & ) = delete;
    JobGgmlTensorFiber &operator=( const JobGgmlTensorFiber & ) = delete;
    JobGgmlTensorFiber( JobGgmlTensorFiber && ) = delete;
    JobGgmlTensorFiber &operator=( JobGgmlTensorFiber && ) = delete;

    [[nodiscard]] std::int64_t length() const noexcept;
    [[nodiscard]] std::size_t elementStride() const noexcept;

    [[nodiscard]] bool isEmpty() const noexcept;
};

} // namespace job::ggml