#include "job_ggml_tensor_batch.h"

namespace job::ggml {

JobGgmlTensorBatch::JobGgmlTensorBatch(struct ggml_tensor *tensor) :
    BatchRank{tensor}
{
}

std::int64_t JobGgmlTensorBatch::width() const noexcept
{
    return extent(0);
}

std::int64_t JobGgmlTensorBatch::height() const noexcept
{
    return extent(1);
}

std::int64_t JobGgmlTensorBatch::depth() const noexcept
{
    return extent(2);
}

std::int64_t JobGgmlTensorBatch::batchCount() const noexcept
{
    return extent(3);
}

std::int64_t JobGgmlTensorBatch::columns() const noexcept
{
    return width();
}

std::int64_t JobGgmlTensorBatch::rows() const noexcept
{
    return height();
}

std::int64_t JobGgmlTensorBatch::planeCount() const noexcept
{
    return depth();
}

std::size_t JobGgmlTensorBatch::elementStride() const noexcept
{
    return stride(0);
}

std::size_t JobGgmlTensorBatch::rowStride() const noexcept
{
    return stride(1);
}

std::size_t JobGgmlTensorBatch::planeStride() const noexcept
{
    return stride(2);
}

std::size_t JobGgmlTensorBatch::batchStride() const noexcept
{
    return stride(3);
}

std::int64_t JobGgmlTensorBatch::elementsPerPlane() const noexcept
{
    if (width() <= 0 || height() <= 0)
        return 0;

    return width() * height();
}

std::int64_t JobGgmlTensorBatch::elementsPerBatch() const noexcept
{
    const std::int64_t planeElements = elementsPerPlane();

    if (planeElements <= 0 || depth() <= 0)
        return 0;

    return planeElements * depth();
}

bool JobGgmlTensorBatch::isSingleBatch() const noexcept
{
    return batchCount() == 1;
}

bool JobGgmlTensorBatch::isEmpty() const noexcept
{
    return width() <= 0 || height() <= 0 || depth() <= 0 || batchCount() <= 0;
}

} // namespace job::ggml