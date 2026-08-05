#include "job_ggml_tensor_fiber.h"
namespace job::ggml {

JobGgmlTensorFiber::JobGgmlTensorFiber(struct ggml_tensor *tensor) :
    FiberRank{tensor}
{
}

std::int64_t JobGgmlTensorFiber::length() const noexcept
{
    return extent(0);
}

std::size_t JobGgmlTensorFiber::elementStride() const noexcept
{
    return stride(0);
}

bool JobGgmlTensorFiber::isEmpty() const noexcept
{
    return length() <= 0;
}

} // job::ggml
