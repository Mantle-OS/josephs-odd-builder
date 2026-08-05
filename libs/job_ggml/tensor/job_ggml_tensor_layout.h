#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <ggml.h>

#include "job_ggml_tensor_extents.h"
#include "jobggml_export.h"

namespace job::ggml {

enum class JobGgmlTensorLayoutType : std::uint8_t {
    Unknown     = 0,
    Contiguous  = 1,
    Transposed  = 2,
    Permuted    = 3,
    Strided     = 4
};

class JOBGGML_EXPORT JobGgmlTensorLayout
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorLayout>;
    using UPtr = std::unique_ptr<JobGgmlTensorLayout>;

    static constexpr std::size_t MaxRank = GGML_MAX_DIMS;

    explicit JobGgmlTensorLayout(struct ggml_tensor *tensor);
    ~JobGgmlTensorLayout() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorLayout>(tensor); }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorLayout>(tensor); }

    JobGgmlTensorLayout(const JobGgmlTensorLayout &) = delete;
    JobGgmlTensorLayout &operator=(const JobGgmlTensorLayout &) = delete;
    JobGgmlTensorLayout(JobGgmlTensorLayout &&) = delete;
    JobGgmlTensorLayout &operator=(JobGgmlTensorLayout &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] JobGgmlTensorLayoutType layoutType() const noexcept;

    /*
     * The tensor can be traversed with a single flattened index without
     * gaps or dimension permutation.
     */
    [[nodiscard]] bool isContiguous() const noexcept;

    /*
     * Partial contiguity checks supplied by GGML:
     *
     * isContiguous0() == ordinary full contiguity
     * isContiguous1() == dimensions 1 and above are contiguous
     * isContiguous2() == dimensions 2 and above are contiguous
     */
    [[nodiscard]] bool isContiguous0() const noexcept;
    [[nodiscard]] bool isContiguous1() const noexcept;
    [[nodiscard]] bool isContiguous2() const noexcept;

    /*
     * The tensor occupies one contiguous block of memory. A permutation may
     * still prevent flattened logical traversal.
     */
    [[nodiscard]] bool isContiguouslyAllocated() const noexcept;

    /*
     * Specialized GGML layout inspections.
     */
    [[nodiscard]] bool hasContiguousChannels() const noexcept;
    [[nodiscard]] bool hasContiguousRows() const noexcept;

    [[nodiscard]] bool isTransposed() const noexcept;
    [[nodiscard]] bool isPermuted() const noexcept;

    /*
     * A tensor is considered generally strided here when it cannot be
     * flattened contiguously and is not identified as a direct transpose or
     * permutation by GGML.
     */
    [[nodiscard]] bool isStrided() const noexcept;

    /*
     * True when the tensor's logical traversal contains gaps in its physical
     * storage. A permuted tensor may be non-contiguous while still occupying
     * one gap-free allocation, so that case returns false here.
     */
    [[nodiscard]] bool hasStorageGaps() const noexcept;

    /*
     * Padding added after the logical tensor storage to satisfy GGML memory
     * alignment. This does not attempt to measure internal row or plane gaps.
     */
    [[nodiscard]] bool hasTrailingPadding() const noexcept;
    [[nodiscard]] std::size_t trailingPaddingBytes() const noexcept;

    [[nodiscard]] int rank() const noexcept;

    [[nodiscard]] std::int64_t extent(std::size_t dimension) const noexcept;

    [[nodiscard]] std::size_t stride(std::size_t dimension) const noexcept;

    [[nodiscard]] std::array<std::int64_t, MaxRank> extents() const noexcept;

    [[nodiscard]] std::array<std::size_t, MaxRank> strides() const noexcept;

    /*
     * Returns true when a dimension contains exactly one element. Such a
     * dimension can often participate in broadcasting/repetition, although
     * this method does not itself prove operation compatibility.
     */
    [[nodiscard]] bool isSingletonDimension(
        std::size_t dimension
        ) const noexcept;

    [[nodiscard]] std::size_t singletonDimensionCount() const noexcept;
    [[nodiscard]] bool hasSingletonDimensions() const noexcept;

    [[nodiscard]] bool hasSameShape(const JobGgmlTensorLayout &other) const noexcept;

    [[nodiscard]] bool hasSameStride(const JobGgmlTensorLayout &other) const noexcept;

    [[nodiscard]] bool hasSameLayout(const JobGgmlTensorLayout &other) const noexcept;

    [[nodiscard]] JobGgmlTensorExtents *tensorExtents() noexcept;
    [[nodiscard]] const JobGgmlTensorExtents *tensorExtents() const noexcept;

    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

private:
    [[nodiscard]] bool validDimension(std::size_t dimension) const noexcept;

    struct ggml_tensor        *m_tensor{nullptr}; // Borrowed from the owning GGML context.
    JobGgmlTensorExtents::UPtr m_extents;
};

} // namespace job::ggml