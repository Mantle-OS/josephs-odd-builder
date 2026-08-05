#include "job_ggml_tensor_volume.h"

namespace job::ggml {

JobGgmlTensorVolume::JobGgmlTensorVolume(struct ggml_tensor *tensor) :
    VolumeRank{tensor}
{
}

std::int64_t JobGgmlTensorVolume::width() const noexcept
{
    return extent(0);
}

std::int64_t JobGgmlTensorVolume::height() const noexcept
{
    return extent(1);
}

std::int64_t JobGgmlTensorVolume::depth() const noexcept
{
    return extent(2);
}

std::int64_t JobGgmlTensorVolume::rows() const noexcept
{
    return height();
}

std::int64_t JobGgmlTensorVolume::columns() const noexcept
{
    return width();
}

std::int64_t JobGgmlTensorVolume::planeCount() const noexcept
{
    return depth();
}

std::size_t JobGgmlTensorVolume::elementStride() const noexcept
{
    return stride(0);
}

std::size_t JobGgmlTensorVolume::rowStride() const noexcept
{
    return stride(1);
}

std::size_t JobGgmlTensorVolume::planeStride() const noexcept
{
    return stride(2);
}

std::int64_t JobGgmlTensorVolume::elementsPerPlane() const noexcept
{
    if (width() <= 0 || height() <= 0)
        return 0;

    return width() * height();
}

bool JobGgmlTensorVolume::isCube() const noexcept
{
    return width() > 0 && width() == height() && height() == depth();
}

bool JobGgmlTensorVolume::isEmpty() const noexcept
{
    return width() <= 0 || height() <= 0 || depth() <= 0;
}

} // namespace job::ggml