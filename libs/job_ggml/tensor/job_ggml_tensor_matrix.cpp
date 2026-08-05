#include "job_ggml_tensor_matrix.h"

namespace job::ggml {

JobGgmlTensorMatrix::JobGgmlTensorMatrix(struct ggml_tensor *tensor) :
    MatrixRank{tensor}
{
}

std::int64_t JobGgmlTensorMatrix::rows() const noexcept
{
    return extent(1);
}

std::int64_t JobGgmlTensorMatrix::columns() const noexcept
{
    return extent(0);
}

std::size_t JobGgmlTensorMatrix::elementStride() const noexcept
{
    return stride(0);
}

std::size_t JobGgmlTensorMatrix::rowStride() const noexcept
{
    return stride(1);
}

bool JobGgmlTensorMatrix::isSquare() const noexcept
{
    return rows() > 0 && rows() == columns();
}

bool JobGgmlTensorMatrix::isEmpty() const noexcept
{
    return rows() <= 0 || columns() <= 0;
}

} // namespace job::ggml