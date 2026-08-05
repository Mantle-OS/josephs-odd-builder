#pragma once

#include <cstdint>
#include <memory>

#include <ggml.h>

#include "job_ggml_tensor_rank.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorVolume final : public VolumeRank
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorVolume>;
    using UPtr = std::unique_ptr<JobGgmlTensorVolume>;

    explicit JobGgmlTensorVolume(struct ggml_tensor *tensor);
    ~JobGgmlTensorVolume() override = default;

    [[nodiscard]] static Ptr createShared( struct ggml_tensor *tensor ) { return std::make_shared<JobGgmlTensorVolume>( tensor ); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor){ return std::make_unique<JobGgmlTensorVolume>(tensor);}

    JobGgmlTensorVolume(const JobGgmlTensorVolume &) = delete;
    JobGgmlTensorVolume &operator=(const JobGgmlTensorVolume &) = delete;
    JobGgmlTensorVolume(JobGgmlTensorVolume &&) = delete;
    JobGgmlTensorVolume &operator=(JobGgmlTensorVolume &&) = delete;

    [[nodiscard]] std::int64_t width() const noexcept;
    [[nodiscard]] std::int64_t height() const noexcept;
    [[nodiscard]] std::int64_t depth() const noexcept;

    [[nodiscard]] std::int64_t rows() const noexcept;
    [[nodiscard]] std::int64_t columns() const noexcept;
    [[nodiscard]] std::int64_t planeCount() const noexcept;

    [[nodiscard]] std::size_t elementStride() const noexcept;
    [[nodiscard]] std::size_t rowStride() const noexcept;
    [[nodiscard]] std::size_t planeStride() const noexcept;

    [[nodiscard]] std::int64_t elementsPerPlane() const noexcept;

    [[nodiscard]] bool isCube() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
};

} // namespace job::ggml