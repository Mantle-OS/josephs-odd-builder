#include "job_ggml_tensor_layout.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlTensorLayout::JobGgmlTensorLayout(struct ggml_tensor *tensor) :
    m_tensor{tensor},
    m_extents{JobGgmlTensorExtents::createUniq(tensor)}
{
     // JobGgmlTensorExtents validates the pointer during construction, so no second nullptr check is required here.
}

bool JobGgmlTensorLayout::isValid() const noexcept
{
    return m_tensor != nullptr &&
           m_extents &&
           m_extents->isValid();
}

JobGgmlTensorLayoutType JobGgmlTensorLayout::layoutType() const noexcept
{
    if (!isValid())
        return JobGgmlTensorLayoutType::Unknown;

    if (isContiguous())
        return JobGgmlTensorLayoutType::Contiguous;

    if (isTransposed())
        return JobGgmlTensorLayoutType::Transposed;

    if (isPermuted())
        return JobGgmlTensorLayoutType::Permuted;

    return JobGgmlTensorLayoutType::Strided;
}

bool JobGgmlTensorLayout::isContiguous() const noexcept
{
    return m_tensor && ggml_is_contiguous(m_tensor);
}

bool JobGgmlTensorLayout::isContiguous0() const noexcept
{
    return m_tensor && ggml_is_contiguous_0(m_tensor);
}

bool JobGgmlTensorLayout::isContiguous1() const noexcept
{
    return m_tensor && ggml_is_contiguous_1(m_tensor);
}

bool JobGgmlTensorLayout::isContiguous2() const noexcept
{
    return m_tensor && ggml_is_contiguous_2(m_tensor);
}

bool JobGgmlTensorLayout::isContiguouslyAllocated() const noexcept
{
    return m_tensor && ggml_is_contiguously_allocated(m_tensor);
}

bool JobGgmlTensorLayout::hasContiguousChannels() const noexcept
{
    return m_tensor && ggml_is_contiguous_channels(m_tensor);
}

bool JobGgmlTensorLayout::hasContiguousRows() const noexcept
{
    return m_tensor && ggml_is_contiguous_rows(m_tensor);
}

bool JobGgmlTensorLayout::isTransposed() const noexcept
{
    return m_tensor && ggml_is_transposed(m_tensor);
}

bool JobGgmlTensorLayout::isPermuted() const noexcept
{
    return m_tensor && ggml_is_permuted(m_tensor);
}

bool JobGgmlTensorLayout::isStrided() const noexcept
{
    return isValid() &&
           !isContiguous() &&
           !isTransposed() &&
           !isPermuted();
}

bool JobGgmlTensorLayout::hasStorageGaps() const noexcept
{
    if (!isValid())
        return false;

    return !isContiguouslyAllocated();
}

bool JobGgmlTensorLayout::hasTrailingPadding() const noexcept
{
    return trailingPaddingBytes() > 0;
}

std::size_t JobGgmlTensorLayout::trailingPaddingBytes() const noexcept
{
    if (!m_tensor)
        return 0;

    const std::size_t byteCount =
        ggml_nbytes(m_tensor);

    const std::size_t paddedByteCount =
        ggml_nbytes_pad(m_tensor);

    return paddedByteCount >= byteCount ? paddedByteCount - byteCount : 0;
}

int JobGgmlTensorLayout::rank() const noexcept
{
    return m_extents ? m_extents->rank() : 0;
}

std::int64_t JobGgmlTensorLayout::extent(std::size_t dimension) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_extents->extent(dimension);
}

std::size_t JobGgmlTensorLayout::stride(std::size_t dimension) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_extents->stride(dimension);
}

std::array<std::int64_t, JobGgmlTensorLayout::MaxRank> JobGgmlTensorLayout::extents() const noexcept
{
    return m_extents ? m_extents->extents() : std::array<std::int64_t, MaxRank>{};
}

std::array<std::size_t, JobGgmlTensorLayout::MaxRank> JobGgmlTensorLayout::strides() const noexcept
{
    return m_extents ? m_extents->strides() : std::array<std::size_t, MaxRank>{};
}

bool JobGgmlTensorLayout::isSingletonDimension(std::size_t dimension) const noexcept
{
    return validDimension(dimension) && extent(dimension) == 1;
}

std::size_t JobGgmlTensorLayout::singletonDimensionCount() const noexcept
{
    if (!isValid())
        return 0;

    std::size_t count = 0;
    const int tensorRank = rank();
    for (int i = 0; i < tensorRank; ++i) {
        if (isSingletonDimension( static_cast<std::size_t>(i))) {
            ++count;
        }
    }

    return count;
}

bool JobGgmlTensorLayout::hasSingletonDimensions() const noexcept
{
    return singletonDimensionCount() > 0;
}

bool JobGgmlTensorLayout::hasSameShape(const JobGgmlTensorLayout &other) const noexcept
{
    if (!isValid() || !other.isValid())
        return false;

    return ggml_are_same_shape(m_tensor, other.m_tensor);
}

bool JobGgmlTensorLayout::hasSameStride(const JobGgmlTensorLayout &other) const noexcept
{
    if (!isValid() || !other.isValid())
        return false;

    return ggml_are_same_stride(m_tensor, other.m_tensor);
}

bool JobGgmlTensorLayout::hasSameLayout(const JobGgmlTensorLayout &other) const noexcept
{
    return hasSameShape(other) && hasSameStride(other);
}

JobGgmlTensorExtents *JobGgmlTensorLayout::tensorExtents() noexcept
{
    return m_extents.get();
}

const JobGgmlTensorExtents *JobGgmlTensorLayout::tensorExtents() const noexcept
{
    return m_extents.get();
}

struct ggml_tensor *JobGgmlTensorLayout::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensorLayout::tensor() const noexcept
{
    return m_tensor;
}

bool JobGgmlTensorLayout::validDimension(std::size_t dimension) const noexcept
{
    return m_extents &&
           m_extents->isValid() &&
           dimension < static_cast<std::size_t>(rank()) &&
           dimension < MaxRank;
}

} // namespace job::ggml