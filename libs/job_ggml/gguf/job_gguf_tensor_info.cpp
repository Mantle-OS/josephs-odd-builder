#include "job_gguf_tensor_info.h"

#include <limits>
#include <stdexcept>

namespace job::ggml {

JobGgufTensorInfo::JobGgufTensorInfo(const JobGgmlTensor &tensor, std::uint64_t offset) :
    m_offset{offset}
{
    setTensor(tensor);
}

bool JobGgufTensorInfo::isValid() const noexcept
{
    return m_tensor.name[0] != '\0' &&
           m_tensor.type >= 0 &&
           m_tensor.type < GGML_TYPE_COUNT &&
           m_tensor.ne[0] > 0;
}

std::string JobGgufTensorInfo::name() const
{
    return m_tensor.name;
}

void JobGgufTensorInfo::setName(const std::string &name)
{
    if (name.empty()) {
        throw std::invalid_argument{
            "JobGgufTensorInfo requires a non-empty tensor name"
        };
    }

    if (name.size() >= GGML_MAX_NAME) {
        throw std::invalid_argument{
            "JobGgufTensorInfo tensor name exceeds GGML_MAX_NAME"
        };
    }

    ggml_set_name(&m_tensor, name.c_str());
}

JobGgmlType JobGgufTensorInfo::type() const noexcept
{
    return static_cast<JobGgmlType>(m_tensor.type);
}

enum ggml_type JobGgufTensorInfo::ggmlType() const noexcept
{
    return m_tensor.type;
}

void JobGgufTensorInfo::setType(JobGgmlType type)
{
    setGgmlType(static_cast<enum ggml_type>(type));
}

void JobGgufTensorInfo::setGgmlType(enum ggml_type type)
{
    if (type < 0 || type >= GGML_TYPE_COUNT) {
        throw std::invalid_argument{
            "JobGgufTensorInfo received an invalid ggml_type"
        };
    }

    const std::int64_t blockSize = ggml_blck_size(type);
    const std::size_t typeSize = ggml_type_size(type);

    if (blockSize <= 0 || typeSize == 0) {
        throw std::invalid_argument{
            "JobGgufTensorInfo received an unsupported ggml_type"
        };
    }

    if (m_tensor.ne[0] <= 0 ||
        m_tensor.ne[0] % blockSize != 0) {
        throw std::invalid_argument{
            "JobGgufTensorInfo row extent is not divisible by the GGML block size"
        };
    }

    m_tensor.type  = type;
    m_tensor.nb[0] = typeSize;
    m_tensor.nb[1] = m_tensor.nb[0] * static_cast<std::size_t>(m_tensor.ne[0] / blockSize);

    for (std::size_t dim = 2; dim < GGML_MAX_DIMS; ++dim)
        m_tensor.nb[dim] = m_tensor.nb[dim - 1] * static_cast<std::size_t>(m_tensor.ne[dim - 1]);
}

int JobGgufTensorInfo::rank() const noexcept
{
    return isValid() ? ggml_n_dims(&m_tensor) : 0;
}

std::int64_t JobGgufTensorInfo::extent(std::size_t dimension) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_tensor.ne[dimension];
}

std::size_t JobGgufTensorInfo::stride(std::size_t dimension) const noexcept
{
    if (!validDimension(dimension))
        return 0;

    return m_tensor.nb[dimension];
}

std::int64_t JobGgufTensorInfo::elementCount() const noexcept
{
    return isValid() ? ggml_nelements(&m_tensor) : 0;
}

std::size_t JobGgufTensorInfo::byteCount() const noexcept
{
    return isValid() ? ggml_nbytes(&m_tensor) : 0;
}

std::size_t JobGgufTensorInfo::paddedByteCount(std::size_t alignment) const noexcept
{
    if (!isValid() || alignment == 0 || (alignment & (alignment - 1)) != 0)
        return 0;

    const std::size_t bytes = byteCount();
    const std::size_t mask  = alignment - 1;

    if (bytes > std::numeric_limits<std::size_t>::max() - mask)
        return 0;

    return (bytes + mask) & ~mask;
}

bool JobGgufTensorInfo::isQuantized() const noexcept
{
    return isValid() && ggml_is_quantized(m_tensor.type);
}

std::uint64_t JobGgufTensorInfo::offset() const noexcept
{
    return m_offset;
}

void JobGgufTensorInfo::setOffset(std::uint64_t offset) noexcept
{
    if (m_offset != offset)
        m_offset = offset;
}

bool JobGgufTensorInfo::isAligned(std::size_t alignment) const noexcept
{
    return alignment != 0 && (alignment & (alignment - 1)) == 0 && m_offset % alignment == 0;
}

void JobGgufTensorInfo::setTensor(const JobGgmlTensor &tensor)
{
    if (!tensor.isValid() || !tensor.tensor()) {
        throw std::invalid_argument{
            "JobGgufTensorInfo requires a valid JobGgmlTensor"
        };
    }

    m_tensor = *tensor.tensor();
}

void JobGgufTensorInfo::reset() noexcept
{
    m_tensor = {};
    m_offset = 0;
}

bool JobGgufTensorInfo::validDimension(std::size_t dimension) const noexcept
{
    return isValid() && dimension < GGML_MAX_DIMS;
}

} // namespace job::ggml
