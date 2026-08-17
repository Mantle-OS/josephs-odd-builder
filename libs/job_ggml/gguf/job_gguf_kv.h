#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gguf.h>

#include <real_type.h>

#include "job_ggml_enums.h"
#include "job_gguf_type_traits.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgufKv
{
public:
    using Ptr  = std::shared_ptr<JobGgufKv>;
    using WPtr = std::weak_ptr<JobGgufKv>;
    using UPtr = std::unique_ptr<JobGgufKv>;

    template<JobGgufValueType T>
    explicit JobGgufKv(std::string key, const T &value) :
        m_key{std::move(key)}
    {
        setValue(value);
    }

    template<JobGgufValueType T>
    explicit JobGgufKv(std::string key, const std::vector<T> &values) :
        m_key{std::move(key)}
    {
        setValues(values);
    }

    ~JobGgufKv() = default;

    template<JobGgufValueType T>
    [[nodiscard]] static Ptr createShared(std::string key, const T &value)
    {
        return std::make_shared<JobGgufKv>(std::move(key), value);
    }

    template<JobGgufValueType T>
    [[nodiscard]] static Ptr createShared(std::string key, const std::vector<T> &values)
    {
        return std::make_shared<JobGgufKv>(std::move(key), values);
    }

    template<JobGgufValueType T>
    [[nodiscard]] static UPtr createUniq(std::string key, const T &value)
    {
        return std::make_unique<JobGgufKv>(std::move(key), value);
    }

    template<JobGgufValueType T>
    [[nodiscard]] static UPtr createUniq(std::string key, const std::vector<T> &values)
    {
        return std::make_unique<JobGgufKv>(std::move(key), values);
    }

    JobGgufKv(const JobGgufKv &) = delete;
    JobGgufKv &operator=(const JobGgufKv &) = delete;
    JobGgufKv(JobGgufKv &&) = delete;
    JobGgufKv &operator=(JobGgufKv &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] const std::string &key() const noexcept;
    void setKey(const std::string &key);

    // Value shape and type
    [[nodiscard]] bool isArray() const noexcept;
    [[nodiscard]] bool isScalar() const noexcept;

    [[nodiscard]] JobGgufType type() const noexcept;
    [[nodiscard]] enum gguf_type ggufType() const noexcept;

    [[nodiscard]] JobGgufType serializedType() const noexcept;
    [[nodiscard]] enum gguf_type serializedGgufType() const noexcept;

    [[nodiscard]] std::string_view typeName() const noexcept;


    [[nodiscard]] int64_t readScalarInt(int64_t def = -1) const noexcept
    {
        if (isScalar())
            return def;

        switch (type()) {
        case ggml::JobGgufType::UInt8:
            return static_cast<int64_t>(value<uint8_t>());
        case ggml::JobGgufType::Int8:
            return static_cast<int64_t>(value<int8_t>());
        case ggml::JobGgufType::UInt16:
            return static_cast<int64_t>(value<uint16_t>());
        case ggml::JobGgufType::Int16:
            return static_cast<int64_t>(value<int16_t>());
        case ggml::JobGgufType::UInt32:
            return static_cast<int64_t>(value<uint32_t>());
        case ggml::JobGgufType::Int32:
            return static_cast<int64_t>(value<int32_t>());
        case ggml::JobGgufType::UInt64:
            return static_cast<int64_t>(value<uint64_t>());
        case ggml::JobGgufType::Int64:
            return value<int64_t>();
        default:
            return def;
        }
    }

    [[nodiscard]] bool isString() const noexcept;
    [[nodiscard]] std::string readString(const std::string &def = {}) const
    {
        if (isScalar() && isString())
            return value<std::string>();

        return def;
    }

    [[nodiscard]] bool isBoolean() const noexcept;
    [[nodiscard]] bool readBool(bool def = false) const noexcept
    {
        if (!isScalar())
            return def;

        if (isBoolean())
            return value<bool>();

        if (isInteger())
            return readScalarInt(def ? 1 : 0) != 0;

        return def;
    }

    [[nodiscard]] bool isInteger() const noexcept;
    [[nodiscard]] std::int64_t readInt(std::int64_t def = -1) const noexcept
    {
        if (!isScalar() || !isInteger())
            return def;

        return readScalarInt(def);
    }


    [[nodiscard]] bool isFloatingPoint() const noexcept;
    [[nodiscard]] float readFloat(float def = core::safeInfinity()) const noexcept
    {
        if (!isScalar() || !isFloatingPoint())
            return def;

        switch (type()) {
        case ggml::JobGgufType::Float32:
            return value<float>();

        case ggml::JobGgufType::Float64:
            return static_cast<float>(value<double>());

        default:
            return def;
        }
    }


    [[nodiscard]] std::size_t elementCount() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] std::size_t dataByteCount() const noexcept;

    [[nodiscard]] JobGgufTypeTraits::UPtr typeTraits() const;

    template<JobGgufValueType T>
    [[nodiscard]] bool isType() const noexcept
    {
        return m_type == JobGgufTypeTraits::typeFor<T>();
    }

    template<JobGgufValueType T>
    [[nodiscard]] std::remove_cvref_t<T> value() const
    {
        using Value = std::remove_cvref_t<T>;
        if (!isScalar())
            throw std::invalid_argument{ "JobGgufKv::value requires a scalar value"};

        return value<Value>(0);
    }

    template<JobGgufValueType T>
    [[nodiscard]] std::remove_cvref_t<T> value(std::size_t index) const
    {
        using Value = std::remove_cvref_t<T>;

        validateAccess<Value>(index);

        if constexpr (std::is_same_v<Value, std::string>) {
            return m_stringData[index];
        } else if constexpr (std::is_same_v<Value, bool>) {
            return m_data[index] != std::byte{0};
        } else {
            Value ret{};
            std::memcpy(&ret, m_data.data() + index * sizeof(Value), sizeof(Value));
            return ret;
        }
    }

    template<JobGgufValueType T>
    [[nodiscard]] std::vector<std::remove_cvref_t<T>> values() const
    {
        using Value = std::remove_cvref_t<T>;

        if (!isArray())
            throw std::invalid_argument{"JobGgufKv::values requires an array value"};

        if (!isType<Value>())
            throw std::invalid_argument{ "JobGgufKv requested value type does not match the stored GGUF type" };

        std::vector<Value> ret;
        ret.reserve(elementCount());

        for (std::size_t index = 0; index < elementCount(); ++index)
            ret.push_back(value<Value>(index));

        return ret;
    }

    // Typed mutation
    template<JobGgufValueType T>
    void setValue(const T &value)
    {
        using Value = std::remove_cvref_t<T>;
        validateKey();
        clearValueStorage();

        m_isArray = false;
        m_type    = JobGgufTypeTraits::typeFor<Value>();

        if constexpr (std::is_same_v<Value, std::string>) {

            m_stringData.push_back(value);

        } else if constexpr (std::is_same_v<Value, bool>) {

            m_data.resize(sizeof(std::int8_t));
            m_data[0] = value ? std::byte{1} : std::byte{0};

        } else {
            static_assert(std::is_trivially_copyable_v<Value>, "GGUF fixed-width values must be trivially copyable");

            m_data.resize(sizeof(Value));

            std::memcpy(m_data.data(), &value, sizeof(Value));
        }
    }



    template<JobGgufValueType T>
    void setValues(const std::vector<T> &values)
    {
        using Value = std::remove_cvref_t<T>;
        validateKey();
        clearValueStorage();

        m_isArray = true;
        m_type    = JobGgufTypeTraits::typeFor<Value>();

        if constexpr (std::is_same_v<Value, std::string>) {

            m_stringData = values;

        } else if constexpr (std::is_same_v<Value, bool>) {
            m_data.resize(values.size());

            for (std::size_t index = 0; index < values.size(); ++index)
                m_data[index] = values[index] ? std::byte{1} : std::byte{0};

        } else {
            static_assert(std::is_trivially_copyable_v<Value>, "GGUF fixed-width values must be trivially copyable");

            m_data.resize(values.size() * sizeof(Value));

            if (!values.empty())
                std::memcpy(m_data.data(), values.data(), m_data.size());
        }
    }

    void cast(JobGgufType type);
    void cast(enum gguf_type type);

    // Serializer/deserializer storage access
    [[nodiscard]] const std::vector<std::byte> &data() const noexcept;

    [[nodiscard]] const std::vector<std::string> &
    stringData() const noexcept;

    void reset();

private:
    void validateKey() const;
    void clearValueStorage() noexcept;

    template<JobGgufValueType T>
    void validateAccess(std::size_t index) const
    {
        using Value = std::remove_cvref_t<T>;

        if (!isValid()) {
            throw std::runtime_error{
                "Cannot read an invalid JobGgufKv"
            };
        }

        if (!isType<Value>()) {
            throw std::invalid_argument{
                "JobGgufKv requested value type does not match the stored GGUF type"
            };
        }

        if (index >= elementCount()) {
            throw std::out_of_range{
                "JobGgufKv value index is out of range"
            };
        }
    }

    std::string                 m_key;
    bool                        m_isArray{false};
    JobGgufType                 m_type{JobGgufType::UInt8};
    std::vector<std::byte>      m_data;
    std::vector<std::string>    m_stringData;
};

} // namespace job::ggml