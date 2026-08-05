#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <ggml.h>

#include "jobggml_export.h"
namespace job::ggml {

class JOBGGML_EXPORT JobGgmlTensorExtents
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorExtents>;
    using UPtr = std::unique_ptr<JobGgmlTensorExtents>;

    static constexpr std::size_t MaxRank = GGML_MAX_DIMS;

    explicit JobGgmlTensorExtents(struct ggml_tensor *tensor);
    ~JobGgmlTensorExtents() = default;

    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor) { return std::make_shared<JobGgmlTensorExtents>(tensor); }
    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor) { return std::make_unique<JobGgmlTensorExtents>(tensor); }
    JobGgmlTensorExtents(const JobGgmlTensorExtents &) = delete;
    JobGgmlTensorExtents &operator=(const JobGgmlTensorExtents &) = delete;
    JobGgmlTensorExtents(JobGgmlTensorExtents &&) = delete;
    JobGgmlTensorExtents &operator=(JobGgmlTensorExtents &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] int rank() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] std::int64_t extent(std::size_t dimension) const noexcept;
    [[nodiscard]] std::size_t stride(std::size_t dimension) const noexcept;

    [[nodiscard]] std::int64_t ne0() const noexcept;
    [[nodiscard]] std::int64_t ne1() const noexcept;
    [[nodiscard]] std::int64_t ne2() const noexcept;
    [[nodiscard]] std::int64_t ne3() const noexcept;

    [[nodiscard]] std::size_t nb0() const noexcept;
    [[nodiscard]] std::size_t nb1() const noexcept;
    [[nodiscard]] std::size_t nb2() const noexcept;
    [[nodiscard]] std::size_t nb3() const noexcept;

    [[nodiscard]] std::array<std::int64_t, MaxRank> extents() const noexcept;
    [[nodiscard]] std::array<std::size_t, MaxRank> strides() const noexcept;

    [[nodiscard]] std::int64_t volume() const noexcept;
    [[nodiscard]] std::int64_t elementCount() const noexcept;

    [[nodiscard]] std::size_t byteCount() const noexcept;
    [[nodiscard]] std::size_t paddedByteCount() const noexcept;

    [[nodiscard]] std::int64_t rowCount() const noexcept;

    [[nodiscard]] bool isScalar() const noexcept;

    [[nodiscard]] bool isGgmlVectorCompatible() const noexcept;
    [[nodiscard]] bool isGgmlMatrixCompatible() const noexcept;
    [[nodiscard]] bool isGgmlThreeDimensionalCompatible() const noexcept;

    [[nodiscard]] bool isVector() const noexcept;
    [[nodiscard]] bool isMatrix() const noexcept;
    [[nodiscard]] bool isThreeDimensional() const noexcept;
    [[nodiscard]] bool isFourDimensional() const noexcept;

    [[nodiscard]] bool isContiguous() const noexcept;
    [[nodiscard]] bool isContiguous0() const noexcept;
    [[nodiscard]] bool isContiguous1() const noexcept;
    [[nodiscard]] bool isContiguous2() const noexcept;

    [[nodiscard]] bool isTransposed() const noexcept;
    [[nodiscard]] bool isPermuted() const noexcept;

    [[nodiscard]] bool hasSameShape(const JobGgmlTensorExtents &other) const noexcept;

    [[nodiscard]] bool canRepeatTo(const JobGgmlTensorExtents &destination) const noexcept;
    [[nodiscard]] bool canMultiplyMatricesWith(const JobGgmlTensorExtents &other) const noexcept;

    [[nodiscard]] struct ggml_tensor *tensor() noexcept;
    [[nodiscard]] const struct ggml_tensor *tensor() const noexcept;

private:
    [[nodiscard]] bool validDimension(std::size_t dimension) const noexcept;

    struct ggml_tensor *m_tensor{nullptr}; // Borrowed from the owning GGML context.
};

} // namespace job::ggml