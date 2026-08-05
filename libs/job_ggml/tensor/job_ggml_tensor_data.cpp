#include "job_ggml_tensor_data.h"

#include <stdexcept>
#include <limits>

namespace job::ggml {

JobGgmlTensorData::JobGgmlTensorData(struct ggml_tensor *tensor) :
    m_tensor{tensor}
{
    if (!m_tensor) {
        throw std::invalid_argument{
            "JobGgmlTensorData requires a valid ggml_tensor"
        };
    }
}

bool JobGgmlTensorData::isValid() const noexcept
{
    return m_tensor != nullptr;
}

JobGgmlType JobGgmlTensorData::type() const noexcept
{
    return fromGgmlType(ggmlType());
}

enum ggml_type JobGgmlTensorData::ggmlType() const noexcept
{
    return m_tensor ? m_tensor->type : GGML_TYPE_F32;
}

const char *JobGgmlTensorData::typeName() const noexcept
{
    if (!m_tensor)
        return "unknown";

    const char *name = ggml_type_name(m_tensor->type);
    return name ? name : "unknown";
}

std::int64_t JobGgmlTensorData::blockSize() const noexcept
{
    return m_tensor ? ggml_blck_size(m_tensor->type) : 0;
}

std::size_t JobGgmlTensorData::typeSize() const noexcept
{
    return m_tensor ? ggml_type_size(m_tensor->type) : 0;
}

std::size_t JobGgmlTensorData::rowSize() const noexcept
{
    if (!m_tensor)
        return 0;

    return ggml_row_size(m_tensor->type, m_tensor->ne[0]);
}

bool JobGgmlTensorData::isQuantized() const noexcept
{
    return m_tensor && ggml_is_quantized(m_tensor->type);
}

JobGgmlTypeTraits::UPtr JobGgmlTensorData::typeTraits() const
{
    if (!m_tensor) {
        throw std::runtime_error{
            "Cannot inspect type traits for an invalid GGML tensor"
        };
    }

    return JobGgmlTypeTraits::createUniq(m_tensor->type);
}

ggml_backend_buffer_t JobGgmlTensorData::buffer() const noexcept
{
    return m_tensor ? m_tensor->buffer : nullptr;
}

ggml_backend_buffer_type_t JobGgmlTensorData::bufferType() const noexcept
{
    const ggml_backend_buffer_t nativeBuffer = buffer();

    return nativeBuffer ? ggml_backend_buffer_get_type(nativeBuffer) : nullptr;
}

bool JobGgmlTensorData::hasBuffer() const noexcept
{
    return buffer() != nullptr;
}

bool JobGgmlTensorData::bufferIsHost() const noexcept
{
    const ggml_backend_buffer_t nativeBuffer = buffer();
    return nativeBuffer && ggml_backend_buffer_is_host(nativeBuffer);
}

void *JobGgmlTensorData::data() noexcept
{
    return m_tensor ? ggml_get_data(m_tensor) : nullptr;
}

const void *JobGgmlTensorData::data() const noexcept
{
    return m_tensor ? ggml_get_data(m_tensor) : nullptr;
}

float *JobGgmlTensorData::dataF32() noexcept
{
    if (!m_tensor ||
        m_tensor->type != GGML_TYPE_F32) {
        return nullptr;
    }

    return ggml_get_data_f32(m_tensor);
}

const float *JobGgmlTensorData::dataF32() const noexcept
{
    if (!m_tensor ||
        m_tensor->type != GGML_TYPE_F32) {
        return nullptr;
    }

    return ggml_get_data_f32(m_tensor);
}

bool JobGgmlTensorData::hasData() const noexcept
{
    return data() != nullptr;
}

std::int32_t JobGgmlTensorData::flags() const noexcept
{
    return m_tensor ? m_tensor->flags : 0;
}

void JobGgmlTensorData::setFlags(std::int32_t flags) noexcept
{
    if (m_tensor &&
        m_tensor->flags != flags) {
        m_tensor->flags = flags;
    }
}

bool JobGgmlTensorData::hasFlag(JobGgmlTensorFlag flag) const noexcept
{
    if (!m_tensor)
        return false;

    const auto nativeFlag = static_cast<std::int32_t>(flag);

    return (m_tensor->flags & nativeFlag) == nativeFlag;
}

void JobGgmlTensorData::addFlag(JobGgmlTensorFlag flag) noexcept
{
    if (!m_tensor)
        return;

    switch (flag) {
    case JobGgmlTensorFlag::Input:
        ggml_set_input(m_tensor);
        break;

    case JobGgmlTensorFlag::Output:
        ggml_set_output(m_tensor);
        break;

    case JobGgmlTensorFlag::Param:
        ggml_set_param(m_tensor);
        break;

    case JobGgmlTensorFlag::Loss:
        ggml_set_loss(m_tensor);
        break;

    case JobGgmlTensorFlag::Compute:
        m_tensor->flags |= GGML_TENSOR_FLAG_COMPUTE;
        break;

    case JobGgmlTensorFlag::None:
        break;
    }
}

void JobGgmlTensorData::removeFlag(JobGgmlTensorFlag flag) noexcept
{
    if (!m_tensor)
        return;

    m_tensor->flags &=
        ~static_cast<std::int32_t>(flag);
}

bool JobGgmlTensorData::isInput() const noexcept
{
    return hasFlag(JobGgmlTensorFlag::Input);
}

bool JobGgmlTensorData::isOutput() const noexcept
{
    return hasFlag(JobGgmlTensorFlag::Output);
}

bool JobGgmlTensorData::isParameter() const noexcept
{
    return hasFlag(JobGgmlTensorFlag::Param);
}

bool JobGgmlTensorData::isLoss() const noexcept
{
    return hasFlag(JobGgmlTensorFlag::Loss);
}

bool JobGgmlTensorData::isCompute() const noexcept
{
    return hasFlag(JobGgmlTensorFlag::Compute);
}

void *JobGgmlTensorData::extra() noexcept
{
    return m_tensor ? m_tensor->extra : nullptr;
}

const void *JobGgmlTensorData::extra() const noexcept
{
    return m_tensor ? m_tensor->extra : nullptr;
}

bool JobGgmlTensorData::hasExtra() const noexcept
{
    return extra() != nullptr;
}

bool JobGgmlTensorData::isHostAccessible() const noexcept
{
    if (!m_tensor || !ggml_get_data(m_tensor))
        return false;


    if (!m_tensor->buffer)
        return true;

    return ggml_backend_buffer_is_host(m_tensor->buffer);
}

void JobGgmlTensorData::setValueI32(int64_t index, int32_t value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "setValueI32 requires host-accessible tensor storage"
        };
    }

    if (!validElementIndex(index)) {
        throw std::out_of_range{
            "setValueI32 tensor index is out of range"
        };
    }

    if (index > std::numeric_limits<int>::max()) {
        throw std::overflow_error{
            "setValueI32 tensor index exceeds the GGML integer index range"
        };
    }

    ggml_set_i32_1d(m_tensor, static_cast<int>(index), value);
}

float JobGgmlTensorData::valueF32(int64_t index) const
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "valueF32 requires host-accessible tensor storage"
        };
    }

    if (!validElementIndex(index)) {
        throw std::out_of_range{
            "valueF32 tensor index is out of range"
        };
    }

    if (index > std::numeric_limits<int>::max()) {
        throw std::overflow_error{
            "valueF32 tensor index exceeds the GGML integer index range"
        };
    }

    return ggml_get_f32_1d(m_tensor, static_cast<int>(index));
}

void JobGgmlTensorData::setValueF32(int64_t index, float value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "setValueF32 requires host-accessible tensor storage"
        };
    }

    if (!validElementIndex(index)) {
        throw std::out_of_range{
            "setValueF32 tensor index is out of range"
        };
    }

    if (index > std::numeric_limits<int>::max()) {
        throw std::overflow_error{
            "setValueF32 tensor index exceeds the GGML integer index range"
        };
    }

    ggml_set_f32_1d(m_tensor, static_cast<int>(index), value);
}

void JobGgmlTensorData::fillF32(float value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "fillF32 requires host-accessible tensor storage"
        };
    }

    ggml_set_f32(m_tensor, value);
}

float JobGgmlTensorData::valueF32(int64_t i0, int64_t i1, int64_t i2, int64_t i3) const
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "valueF32 requires host-accessible tensor storage"
        };
    }

    if (!validElementCoordinates(i0, i1, i2, i3)) {
        throw std::out_of_range{
            "valueF32 tensor coordinates are out of range"
        };
    }

    return ggml_get_f32_nd(m_tensor, static_cast<int>(i0), static_cast<int>(i1), static_cast<int>(i2), static_cast<int>(i3));
}

void JobGgmlTensorData::setValueF32(int64_t i0, int64_t i1, int64_t i2, int64_t i3, float value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "setValueF32 requires host-accessible tensor storage"
        };
    }

    if (!validElementCoordinates(i0, i1, i2, i3)) {
        throw std::out_of_range{
            "setValueF32 tensor coordinates are out of range"
        };
    }

    ggml_set_f32_nd( m_tensor, static_cast<int>(i0), static_cast<int>(i1), static_cast<int>(i2), static_cast<int>(i3), value);
}

void JobGgmlTensorData::fillI32(int32_t value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "fillI32 requires host-accessible tensor storage"
        };
    }

    ggml_set_i32(m_tensor, value);
}

void JobGgmlTensorData::setValueI32(int64_t i0, int64_t i1, int64_t i2, int64_t i3, int32_t value)
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "setValueI32 requires host-accessible tensor storage"
        };
    }

    if (!validElementCoordinates(i0, i1, i2, i3)) {
        throw std::out_of_range{
            "setValueI32 tensor coordinates are out of range"
        };
    }

    ggml_set_i32_nd(m_tensor, static_cast<int>(i0), static_cast<int>(i1), static_cast<int>(i2), static_cast<int>(i3), value);
}

int32_t JobGgmlTensorData::valueI32(int64_t i0, int64_t i1, int64_t i2, int64_t i3) const
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "valueI32 requires host-accessible tensor storage"
        };
    }

    if (!validElementCoordinates(i0, i1, i2, i3)) {
        throw std::out_of_range{
            "valueI32 tensor coordinates are out of range"
        };
    }

    return ggml_get_i32_nd(m_tensor, static_cast<int>(i0), static_cast<int>(i1), static_cast<int>(i2), static_cast<int>(i3));
}

int32_t JobGgmlTensorData::valueI32(int64_t index) const
{
    if (!isHostAccessible()) {
        throw std::runtime_error{
            "valueI32 requires host-accessible tensor storage"
        };
    }

    if (!validElementIndex(index)) {
        throw std::out_of_range{
            "valueI32 tensor index is out of range"
        };
    }

    if (index > std::numeric_limits<int>::max()) {
        throw std::overflow_error{
            "valueI32 tensor index exceeds the GGML integer index range"
        };
    }

    return ggml_get_i32_1d(m_tensor, static_cast<int>(index));
}

struct ggml_tensor *JobGgmlTensorData::tensor() noexcept
{
    return m_tensor;
}

const struct ggml_tensor *JobGgmlTensorData::tensor() const noexcept
{
    return m_tensor;
}

bool JobGgmlTensorData::validElementIndex(std::int64_t index) const noexcept
{
    return m_tensor && index >= 0 && index < ggml_nelements(m_tensor);
}

bool JobGgmlTensorData::validElementCoordinates(int64_t i0, int64_t i1, int64_t i2, int64_t i3) const noexcept
{
    if (!m_tensor)
        return false;

    const std::int64_t coordinates[GGML_MAX_DIMS]{
        i0,
        i1,
        i2,
        i3
    };

    for (std::size_t dimension = 0; dimension < GGML_MAX_DIMS; ++dimension)
        if (coordinates[dimension] < 0 || coordinates[dimension] >= m_tensor->ne[dimension] || coordinates[dimension] > std::numeric_limits<int>::max())
            return false;


    return true;
}

} // namespace job::ggml