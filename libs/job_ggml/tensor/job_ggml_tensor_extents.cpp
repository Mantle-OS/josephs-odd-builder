#include "job_ggml_tensor_extents.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlTensorExtents::JobGgmlTensorExtents(struct ggml_tensor *tensor) :
    m_tensor{tensor}
{
    if (!m_tensor) {
        throw std::invalid_argument{
            "JobGgmlTensorExtents requires a valid ggml_tensor"
        };
    }
}

bool JobGgmlTensorExtents::isValid() const noexcept
{
    return m_tensor != nullptr;
}

int JobGgmlTensorExtents::rank() const noexcept
{
    return m_tensor ? ggml_n_dims(m_tensor) : 0;
}

std::size_t JobGgmlTensorExtents::size() const noexcept
{
    const int tensorRank = rank();
    return tensorRank > 0 ? static_cast<std::size_t>(tensorRank) : 0;
}

std::int64_t JobGgmlTensorExtents::extent(
    std::size_t dimension
    ) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_tensor->ne[dimension];
}

std::size_t JobGgmlTensorExtents::stride(
    std::size_t dimension
    ) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_tensor->nb[dimension];
}

std::int64_t JobGgmlTensorExtents::ne0() const noexcept
{
    return extent(0);
}

std::int64_t JobGgmlTensorExtents::ne1() const noexcept
{
    return extent(1);
}

std::int64_t JobGgmlTensorExtents::ne2() const noexcept
{
    return extent(2);
}

std::int64_t JobGgmlTensorExtents::ne3() const noexcept
{
    return extent(3);
}

std::size_t JobGgmlTensorExtents::nb0() const noexcept
{
    return stride(0);
}

std::size_t JobGgmlTensorExtents::nb1() const noexcept
{
    return stride(1);
}

std::size_t JobGgmlTensorExtents::nb2() const noexcept
{
    return stride(2);
}

std::size_t JobGgmlTensorExtents::nb3() const noexcept
{
    return stride(3);
}

std::array<std::int64_t, JobGgmlTensorExtents::MaxRank> JobGgmlTensorExtents::extents() const noexcept
{
    std::array<std::int64_t, MaxRank> ret{};

    if (!m_tensor)
        return ret;

    for (std::size_t i = 0; i < MaxRank; ++i)
        ret[i] = m_tensor->ne[i];

    return ret;
}

std::array<std::size_t, JobGgmlTensorExtents::MaxRank> JobGgmlTensorExtents::strides() const noexcept
{
    std::array<std::size_t, MaxRank> ret{};

    if (!m_tensor)
        return ret;

    for (std::size_t i = 0; i < MaxRank; ++i)
        ret[i] = m_tensor->nb[i];

    return ret;
}

std::int64_t JobGgmlTensorExtents::volume() const noexcept
{
    return elementCount();
}

std::int64_t JobGgmlTensorExtents::elementCount() const noexcept
{
    return m_tensor ? ggml_nelements(m_tensor) : 0;
}

std::size_t JobGgmlTensorExtents::byteCount() const noexcept
{
    return m_tensor ? ggml_nbytes(m_tensor) : 0;
}

std::size_t JobGgmlTensorExtents::paddedByteCount() const noexcept
{
    return m_tensor ? ggml_nbytes_pad(m_tensor) : 0;
}

std::int64_t JobGgmlTensorExtents::rowCount() const noexcept
{
    return m_tensor ? ggml_nrows(m_tensor) : 0;
}

bool JobGgmlTensorExtents::isScalar() const noexcept
{
    return m_tensor && ggml_is_scalar(m_tensor);
}

bool JobGgmlTensorExtents::isGgmlVectorCompatible() const noexcept
{
    return m_tensor && ggml_is_vector(m_tensor);
}

bool JobGgmlTensorExtents::isGgmlMatrixCompatible() const noexcept
{
    return m_tensor && ggml_is_matrix(m_tensor);
}

bool JobGgmlTensorExtents::isGgmlThreeDimensionalCompatible() const noexcept
{
    return m_tensor && ggml_is_3d(m_tensor);
}

bool JobGgmlTensorExtents::isVector() const noexcept
{
    return rank() == 1;
}

bool JobGgmlTensorExtents::isMatrix() const noexcept
{
    return rank() == 2;
}

bool JobGgmlTensorExtents::isThreeDimensional() const noexcept
{
    return rank() == 3;
}

bool JobGgmlTensorExtents::isFourDimensional() const noexcept
{
    return rank() == 4;
}


// bool JobGgmlTensorExtents::isVector() const noexcept
// {
//     return m_tensor && ggml_is_vector(m_tensor); }

// bool JobGgmlTensorExtents::isMatrix() const noexcept
// {
//     return m_tensor && ggml_is_matrix(m_tensor);
// }

// bool JobGgmlTensorExtents::isThreeDimensional() const noexcept
// {
//     return m_tensor && ggml_is_3d(m_tensor);
// }

// bool JobGgmlTensorExtents::isFourDimensional() const noexcept
// {
//     return rank() == static_cast<int>(MaxRank);
// }

bool JobGgmlTensorExtents::isContiguous() const noexcept
{
    return m_tensor && ggml_is_contiguous(m_tensor);
}

bool JobGgmlTensorExtents::isContiguous0() const noexcept
{
    return m_tensor && ggml_is_contiguous_0(m_tensor);
}

bool JobGgmlTensorExtents::isContiguous1() const noexcept
{
    return m_tensor && ggml_is_contiguous_1(m_tensor);
}

bool JobGgmlTensorExtents::isContiguous2() const noexcept
{
    return m_tensor && ggml_is_contiguous_2(m_tensor);
}

bool JobGgmlTensorExtents::isTransposed() const noexcept
{
    return m_tensor && ggml_is_transposed(m_tensor);
}

bool JobGgmlTensorExtents::isPermuted() const noexcept
{
    return m_tensor && ggml_is_permuted(m_tensor);
}

bool JobGgmlTensorExtents::hasSameShape(const JobGgmlTensorExtents &other) const noexcept
{
    if (!m_tensor || !other.m_tensor)
        return false;

    return ggml_are_same_shape(m_tensor, other.m_tensor);
}

bool JobGgmlTensorExtents::canRepeatTo(const JobGgmlTensorExtents &destination) const noexcept
{
    if (!m_tensor || !destination.m_tensor)
        return false;

    return ggml_can_repeat(m_tensor, destination.m_tensor);
}

bool JobGgmlTensorExtents::canMultiplyMatricesWith(const JobGgmlTensorExtents &other) const noexcept
{
    if (!m_tensor || !other.m_tensor)
        return false;

    /*
     * GGML's public API does not expose its internal can_mul_mat()
     * helper in this revision.
     *
     * ggml_mul_mat(a, b) requires the first dimensions to match,
     * while the outer dimensions of b must be repeat-compatible
     * with those of a.
     */
    if (ne0() != other.ne0())
        return false;

    if (ne2() <= 0 ||
        ne3() <= 0 ||
        other.ne2() <= 0 ||
        other.ne3() <= 0)
    {
        return false;
    }

    return other.ne2() % ne2() == 0 &&
           other.ne3() % ne3() == 0;
}

struct ggml_tensor *JobGgmlTensorExtents::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensorExtents::tensor() const noexcept
{
    return m_tensor;
}

bool JobGgmlTensorExtents::validDimension(std::size_t dimension) const noexcept
{
    return m_tensor && dimension < MaxRank;
}

} // namespace job::ggml