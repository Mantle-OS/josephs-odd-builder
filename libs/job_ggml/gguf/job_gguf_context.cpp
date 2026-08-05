#include "job_gguf_context.h"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgufContext::JobGgufContext() :
    m_context{
    gguf_init_empty()
}
{
    if (!m_context) {
        throw std::runtime_error{
            "Failed to create an empty GGUF context"
        };
    }
}

JobGgufContext::JobGgufContext(struct gguf_context *context) :
    m_context{context}
{
    if (!m_context) {
        throw std::invalid_argument{
            "JobGgufContext requires a valid gguf_context"
        };
    }
}

JobGgufContext::~JobGgufContext() = default;

bool JobGgufContext::isValid() const noexcept
{
    return m_context != nullptr;
}

struct gguf_context *JobGgufContext::context() noexcept
{
    return m_context.get();
}

const struct gguf_context *JobGgufContext::context() const noexcept
{
    return m_context.get();
}

void JobGgufContext::reset() noexcept
{
    m_context.reset();
}

void JobGgufContext::reset(struct gguf_context *context) noexcept
{
    if (m_context.get() == context)
        return;

    m_context.reset(context);
}

std::uint32_t JobGgufContext::version() const noexcept
{
    return m_context ? gguf_get_version(m_context.get()) : 0;
}

std::size_t JobGgufContext::alignment() const noexcept
{
    return m_context ? gguf_get_alignment(m_context.get()) : 0;
}

std::size_t JobGgufContext::dataOffset() const noexcept
{
    return m_context ? gguf_get_data_offset(m_context.get()) : 0;
}

std::size_t JobGgufContext::metadataSize() const noexcept
{
    return m_context ? gguf_get_meta_size(m_context.get()) : 0;
}

std::vector<std::byte> JobGgufContext::metadata() const
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot serialize metadata from an invalid GGUF context"
        };
    }

    const std::size_t size = metadataSize();

    std::vector<std::byte> ret(size);

    if (size > 0)
        gguf_get_meta_data(m_context.get(), ret.data());

    return ret;
}

std::int64_t JobGgufContext::keyValueCount() const noexcept
{
    return m_context ? gguf_get_n_kv(m_context.get()) : 0;
}

bool JobGgufContext::hasKey(const std::string &key) const noexcept
{
    return keyIndex(key) >= 0;
}

std::int64_t JobGgufContext::keyIndex(const std::string &key) const noexcept
{
    if (!m_context || key.empty())
        return -1;

    return gguf_find_key(m_context.get(), key.c_str());
}

std::string JobGgufContext::key(std::int64_t index) const
{
    if (!validKeyIndex(index)) {
        throw std::out_of_range{
            "GGUF key/value index is out of range"
        };
    }

    const char *nativeKey = gguf_get_key(m_context.get(), index);

    if (!nativeKey) {
        throw std::runtime_error{
            "GGUF returned a null key"
        };
    }

    return nativeKey;
}

JobGgufType JobGgufContext::valueType(std::int64_t index) const
{
    return fromGgufType(ggufValueType(index));
}

enum gguf_type JobGgufContext::ggufValueType(std::int64_t index) const
{
    if (!validKeyIndex(index)) {
        throw std::out_of_range{
            "GGUF key/value index is out of range"
        };
    }

    return gguf_get_kv_type(m_context.get(), index);
}

JobGgufType JobGgufContext::arrayElementType(std::int64_t index) const
{
    return fromGgufType(ggufArrayElementType(index));
}

enum gguf_type JobGgufContext::ggufArrayElementType(std::int64_t index) const
{
    if (!validKeyIndex(index)) {
        throw std::out_of_range{
            "GGUF key/value index is out of range"
        };
    }

    const enum gguf_type outerType = gguf_get_kv_type(m_context.get(), index);

    if (outerType != GGUF_TYPE_ARRAY)
        return outerType;

    return gguf_get_arr_type(m_context.get(), index);
}

JobGgufKv::UPtr JobGgufContext::keyValue(std::int64_t index) const
{
    return buildKeyValue(index);
}

JobGgufKv::UPtr JobGgufContext::keyValue(const std::string &key) const
{
    const std::int64_t index = keyIndex(key);
    return index >= 0 ? buildKeyValue(index) : nullptr;
}

std::vector<JobGgufKv::UPtr> JobGgufContext::keyValues() const
{
    std::vector<JobGgufKv::UPtr> ret;
    const std::int64_t count = keyValueCount();

    if (count <= 0)
        return ret;

    ret.reserve(static_cast<std::size_t>(count));

    for (std::int64_t index = 0; index < count; ++index)
        ret.push_back(buildKeyValue(index));

    return ret;
}

std::int64_t JobGgufContext::removeKey(const std::string &key)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot remove a key from an invalid GGUF context"
        };
    }

    if (key.empty()) {
        throw std::invalid_argument{
            "Cannot remove an empty GGUF key"
        };
    }

    return gguf_remove_key(m_context.get(), key.c_str());
}

void JobGgufContext::setKeyValue(const JobGgufKv &keyValue)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot set a key/value pair on an invalid GGUF context"
        };
    }

    if (!keyValue.isValid()) {
        throw std::invalid_argument{
            "JobGgufContext requires a valid JobGgufKv"
        };
    }

    if (keyValue.isArray()) {
        setArrayKeyValue(keyValue);
        return;
    }

    setScalarKeyValue(keyValue);
}

void JobGgufContext::setKeyValues(const JobGgufContext &source)
{
    if (!m_context || !source.isValid()) {
        throw std::invalid_argument{
            "Both GGUF contexts must be valid"
        };
    }

    if (this == &source)
        return;

    gguf_set_kv(m_context.get(), source.context());
}

std::int64_t JobGgufContext::tensorCount() const noexcept
{
    return m_context ? gguf_get_n_tensors(m_context.get()) : 0;
}

bool JobGgufContext::hasTensor(const std::string &name) const noexcept
{
    return tensorIndex(name) >= 0;
}

std::int64_t JobGgufContext::tensorIndex(const std::string &name) const noexcept
{
    if (!m_context || name.empty())
        return -1;

    return gguf_find_tensor(m_context.get(), name.c_str());
}

std::string JobGgufContext::tensorName(std::int64_t index) const
{
    if (!validTensorIndex(index)) {
        throw std::out_of_range{
            "GGUF tensor index is out of range"
        };
    }

    const char *name = gguf_get_tensor_name(m_context.get(), index);
    if (!name) {
        throw std::runtime_error{
            "GGUF returned a null tensor name"
        };
    }

    return name;
}

JobGgmlType JobGgufContext::tensorType(std::int64_t index) const
{
    return static_cast<JobGgmlType>(ggmlTensorType(index));
}

enum ggml_type JobGgufContext::ggmlTensorType(std::int64_t index) const
{
    if (!validTensorIndex(index)) {
        throw std::out_of_range{
            "GGUF tensor index is out of range"
        };
    }

    return gguf_get_tensor_type(m_context.get(), index);
}

std::size_t JobGgufContext::tensorSize(std::int64_t index) const
{
    if (!validTensorIndex(index)) {
        throw std::out_of_range{
            "GGUF tensor index is out of range"
        };
    }

    return gguf_get_tensor_size(m_context.get(), index);
}

std::uint64_t JobGgufContext::tensorOffset(std::int64_t index) const
{
    if (!validTensorIndex(index)) {
        throw std::out_of_range{
            "GGUF tensor index is out of range"
        };
    }

    return static_cast<std::uint64_t>(gguf_get_tensor_offset(m_context.get(), index));
}

void JobGgufContext::addTensor(const JobGgmlTensor &tensor)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot add a tensor to an invalid GGUF context"
        };
    }

    if (!tensor.isValid() || !tensor.tensor()) {
        throw std::invalid_argument{
            "JobGgufContext requires a valid JobGgmlTensor"
        };
    }

    if (!tensor.hasName()) {
        throw std::invalid_argument{
            "A tensor added to GGUF must have a name"
        };
    }

    if (hasTensor(tensor.name())) {
        throw std::invalid_argument{
            "A tensor with the same name already exists in the GGUF context"
        };
    }

    gguf_add_tensor(m_context.get(), tensor.tensor());
}

void JobGgufContext::setTensorType(const std::string &name, JobGgmlType type)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot change a tensor type on an invalid GGUF context"
        };
    }

    if (name.empty() || !hasTensor(name)) {
        throw std::invalid_argument{
            "The requested GGUF tensor does not exist"
        };
    }

    const enum ggml_type nativeType = static_cast<enum ggml_type>(type);

    if (nativeType < 0 || nativeType >= GGML_TYPE_COUNT) {
        throw std::invalid_argument{
            "JobGgufContext received an invalid JobGgmlType"
        };
    }

    gguf_set_tensor_type(m_context.get(), name.c_str(), nativeType);
}

void JobGgufContext::setTensorData(const std::string &name, const void *data)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot set tensor data on an invalid GGUF context"
        };
    }

    if (name.empty() || !hasTensor(name)) {
        throw std::invalid_argument{
            "The requested GGUF tensor does not exist"
        };
    }

    if (!data) {
        throw std::invalid_argument{
            "JobGgufContext requires a valid tensor data pointer"
        };
    }

    gguf_set_tensor_data(m_context.get(), name.c_str(), data);
}

void JobGgufContext::setTensorData(const std::string &name, const std::vector<std::byte> &data)
{
    const std::int64_t index = tensorIndex(name);

    if (index < 0) {
        throw std::invalid_argument{
            "The requested GGUF tensor does not exist"
        };
    }

    const std::size_t requiredSize = tensorSize(index);

    if (data.size() < requiredSize) {
        throw std::invalid_argument{
            "The supplied tensor data buffer is smaller than the GGUF tensor"
        };
    }

    setTensorData(name, data.data());
}

bool JobGgufContext::validKeyIndex(std::int64_t index) const noexcept
{
    return m_context && index >= 0 && index < keyValueCount();
}

bool JobGgufContext::validTensorIndex(std::int64_t index) const noexcept
{
    return m_context && index >= 0 && index < tensorCount();
}

JobGgufKv::UPtr JobGgufContext::buildKeyValue(std::int64_t index) const
{
    if (!validKeyIndex(index)) {
        throw std::out_of_range{
            "GGUF key/value index is out of range"
        };
    }

    const std::string valueKey = key(index);
    const enum gguf_type outerType = gguf_get_kv_type(m_context.get(), index);
    const bool isArray = outerType == GGUF_TYPE_ARRAY;
    const enum gguf_type valueType = isArray ? gguf_get_arr_type(m_context.get(), index) : outerType;
    const std::size_t count = isArray ? gguf_get_arr_n(m_context.get(), index) : 1;

    switch (valueType) {
    case GGUF_TYPE_UINT8:
        if (isArray) {
            const auto *data = static_cast<const std::uint8_t *>(gguf_get_arr_data(m_context.get(), index));
            return JobGgufKv::createUniq(valueKey, std::vector<std::uint8_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_u8(m_context.get(), index));

    case GGUF_TYPE_INT8:
        if (isArray) {
            const auto *data = static_cast<const std::int8_t *>(gguf_get_arr_data(m_context.get(), index));
            return JobGgufKv::createUniq(valueKey, std::vector<std::int8_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_i8(m_context.get(), index));

    case GGUF_TYPE_UINT16:
        if (isArray) {
            const auto *data = static_cast<const std::uint16_t *>(gguf_get_arr_data(m_context.get(), index));
            return JobGgufKv::createUniq(valueKey,std::vector<std::uint16_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_u16(m_context.get(), index));

    case GGUF_TYPE_INT16:
        if (isArray) {
            const auto *data = static_cast<const std::int16_t *>(gguf_get_arr_data(m_context.get(), index));
            return JobGgufKv::createUniq(valueKey, std::vector<std::int16_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(
            valueKey,
            gguf_get_val_i16(m_context.get(), index)
            );

    case GGUF_TYPE_UINT32:
        if (isArray) {
            const auto *data = static_cast<const std::uint32_t *>(
                gguf_get_arr_data(m_context.get(), index)
                );

            return JobGgufKv::createUniq(valueKey, std::vector<std::uint32_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_u32(m_context.get(), index));
    case GGUF_TYPE_INT32:
        if (isArray) {
            const auto *data = static_cast<const std::int32_t *>(gguf_get_arr_data(m_context.get(), index));

            return JobGgufKv::createUniq(valueKey,std::vector<std::int32_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_i32(m_context.get(), index));

    case GGUF_TYPE_FLOAT32:
        if (isArray) {
            const auto *data = static_cast<const float *>(gguf_get_arr_data(m_context.get(), index));
            return JobGgufKv::createUniq(valueKey, std::vector<float>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_f32(m_context.get(), index));

    case GGUF_TYPE_BOOL:
        if (isArray) {
            const auto *data = static_cast<const std::int8_t *>(gguf_get_arr_data(m_context.get(), index));

            std::vector<bool> values;
            values.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
                values.push_back(data[i] != 0);

            return JobGgufKv::createUniq(valueKey, values);
        }

        return JobGgufKv::createUniq(valueKey,gguf_get_val_bool(m_context.get(), index));

    case GGUF_TYPE_STRING:
        if (isArray) {
            std::vector<std::string> values;
            values.reserve(count);

            for (std::size_t i = 0; i < count; ++i) {
                const char *value = gguf_get_arr_str(m_context.get(), index, i);

                values.emplace_back(value ? value : "");
            }

            return JobGgufKv::createUniq(valueKey, values);
        }

        return JobGgufKv::createUniq(valueKey, std::string{ gguf_get_val_str(m_context.get(), index) });

    case GGUF_TYPE_UINT64:
        if (isArray) {
            const auto *data = static_cast<const std::uint64_t *>(gguf_get_arr_data(m_context.get(), index));

            return JobGgufKv::createUniq(valueKey, std::vector<std::uint64_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_u64(m_context.get(), index));

    case GGUF_TYPE_INT64:
        if (isArray) {
            const auto *data = static_cast<const std::int64_t *>(gguf_get_arr_data(m_context.get(), index));

            return JobGgufKv::createUniq(valueKey, std::vector<std::int64_t>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_i64(m_context.get(), index));

    case GGUF_TYPE_FLOAT64:
        if (isArray) {
            const auto *data = static_cast<const double *>(gguf_get_arr_data(m_context.get(), index));

            return JobGgufKv::createUniq(valueKey, std::vector<double>{ data, data + count });
        }

        return JobGgufKv::createUniq(valueKey, gguf_get_val_f64(m_context.get(), index));

    case GGUF_TYPE_ARRAY:
    case GGUF_TYPE_COUNT:
        break;
    }

    throw std::runtime_error{
        "GGUF key/value has an unsupported type"
    };
}

void JobGgufContext::setScalarKeyValue(const JobGgufKv &keyValue)
{
    const char *key = keyValue.key().c_str();

    switch (keyValue.type()) {
    case JobGgufType::UInt8:
        gguf_set_val_u8(m_context.get(), key, keyValue.value<std::uint8_t>());
        return;

    case JobGgufType::Int8:
        gguf_set_val_i8(m_context.get(), key, keyValue.value<std::int8_t>());
        return;

    case JobGgufType::UInt16:
        gguf_set_val_u16(m_context.get(), key, keyValue.value<std::uint16_t>());
        return;

    case JobGgufType::Int16:
        gguf_set_val_i16(m_context.get(), key, keyValue.value<std::int16_t>());
        return;

    case JobGgufType::UInt32:
        gguf_set_val_u32(m_context.get(), key, keyValue.value<std::uint32_t>());
        return;

    case JobGgufType::Int32:
        gguf_set_val_i32(m_context.get(), key, keyValue.value<std::int32_t>());
        return;

    case JobGgufType::Float32:
        gguf_set_val_f32(m_context.get(), key, keyValue.value<float>() );
        return;

    case JobGgufType::Bool:
        gguf_set_val_bool(m_context.get(), key, keyValue.value<bool>());
        return;

    case JobGgufType::String:
    {
        const std::string value = keyValue.value<std::string>();
        gguf_set_val_str(m_context.get(), key, value.c_str());
    }
        return;

    case JobGgufType::UInt64:
        gguf_set_val_u64(m_context.get(), key, keyValue.value<std::uint64_t>());
        return;

    case JobGgufType::Int64:
        gguf_set_val_i64(m_context.get(), key, keyValue.value<std::int64_t>());
        return;

    case JobGgufType::Float64:
        gguf_set_val_f64(m_context.get(), key, keyValue.value<double>());
        return;

    case JobGgufType::Array:
    case JobGgufType::Count:
        break;
    }

    throw std::invalid_argument{
        "JobGgufKv contains an invalid scalar type"
    };
}

void JobGgufContext::setArrayKeyValue(const JobGgufKv &keyValue)
{
    const char *key = keyValue.key().c_str();

    if (keyValue.isString()) {
        const auto &strings = keyValue.stringData();

        std::vector<const char *> pointers;
        pointers.reserve(strings.size());

        for (const std::string &value : strings)
            pointers.push_back(value.c_str());

        gguf_set_arr_str(m_context.get(), key, pointers.data(), pointers.size());

        return;
    }

    gguf_set_arr_data(
        m_context.get(),
        key,
        keyValue.ggufType(),
        keyValue.data().empty() ? nullptr : keyValue.data().data(),
        keyValue.elementCount()
        );
}

void JobGgufContext::GgufContextDeleter::operator()(struct gguf_context *context) const noexcept
{
    if (context)
        gguf_free(context);
}

} // namespace job::ggml