#include "job_gguf_kv.h"

#include <stdexcept>

namespace job::ggml {

bool JobGgufKv::isValid() const noexcept
{
    if (m_key.empty() || !isValidGgufType(m_type) || m_type == JobGgufType::Array)
        return false;

    // A scalar string contains exactly one value. An array may legally contain zero or more strings.
    if (isString()) {
        return m_data.empty() && (m_isArray || m_stringData.size() == 1);
    }

    if (!m_stringData.empty())
        return false;

    const std::size_t elementSize = ggufTypeSize(m_type);

    if (elementSize == 0 || m_data.size() % elementSize != 0)
        return false;

    /*
     * A scalar contains exactly one element. An array may legally contain
     * zero or more elements.
     */
    return m_isArray || m_data.size() == elementSize;
}

const std::string &JobGgufKv::key() const noexcept
{
    return m_key;
}

void JobGgufKv::setKey(const std::string &key)
{
    if (key.empty()) {
        throw std::invalid_argument{
            "JobGgufKv requires a non-empty key"
        };
    }

    if (m_key != key)
        m_key = key;
}

bool JobGgufKv::isArray() const noexcept
{
    return m_isArray;
}

bool JobGgufKv::isScalar() const noexcept
{
    return !m_isArray;
}

JobGgufType JobGgufKv::type() const noexcept
{
    return m_type;
}

enum gguf_type JobGgufKv::ggufType() const noexcept
{
    return toGgufType(m_type);
}

JobGgufType JobGgufKv::serializedType() const noexcept
{
    return m_isArray ? JobGgufType::Array : m_type;
}

enum gguf_type JobGgufKv::serializedGgufType() const noexcept
{
    return toGgufType(serializedType());
}

std::string_view JobGgufKv::typeName() const noexcept
{
    return ggufTypeName(m_type);
}

bool JobGgufKv::isString() const noexcept
{
    return m_type == JobGgufType::String;
}

bool JobGgufKv::isBoolean() const noexcept
{
    return m_type == JobGgufType::Bool;
}

bool JobGgufKv::isInteger() const noexcept
{
    switch (m_type) {
    case JobGgufType::UInt8:
    case JobGgufType::Int8:
    case JobGgufType::UInt16:
    case JobGgufType::Int16:
    case JobGgufType::UInt32:
    case JobGgufType::Int32:
    case JobGgufType::UInt64:
    case JobGgufType::Int64:
        return true;

    default:
        return false;
    }
}

bool JobGgufKv::isFloatingPoint() const noexcept
{
    return m_type == JobGgufType::Float32 || m_type == JobGgufType::Float64;
}

std::size_t JobGgufKv::elementCount() const noexcept
{
    if (isString())
        return m_stringData.size();

    const std::size_t elementSize = ggufTypeSize(m_type);

    if (elementSize == 0 || m_data.size() % elementSize != 0) {
        return 0;
    }

    return m_data.size() / elementSize;
}

bool JobGgufKv::isEmpty() const noexcept
{
    return elementCount() == 0;
}

std::size_t JobGgufKv::dataByteCount() const noexcept
{
    return m_data.size();
}

JobGgufTypeTraits::UPtr JobGgufKv::typeTraits() const
{
    if (!isValidGgufType(m_type) || m_type == JobGgufType::Array) {
        throw std::runtime_error{
            "Cannot inspect type traits for an invalid JobGgufKv"
        };
    }

    return JobGgufTypeTraits::createUniq(m_type);
}

void JobGgufKv::cast(JobGgufType type)
{
    if (!isValidGgufType(type)) {
        throw std::invalid_argument{
            "JobGgufKv received an invalid JobGgufType"
        };
    }

    if (type == JobGgufType::String || type == JobGgufType::Array) {
        throw std::invalid_argument{
            "JobGgufKv cannot cast fixed-width storage to String or Array"
        };
    }

    if (isString()) {
        throw std::invalid_argument{
            "JobGgufKv cannot reinterpret string storage as a fixed-width type"
        };
    }

    const std::size_t newTypeSize = ggufTypeSize(type);

    if (newTypeSize == 0) {
        throw std::invalid_argument{
            "JobGgufKv received a GGUF type without a fixed element size"
        };
    }

    if (m_data.size() % newTypeSize != 0) {
        throw std::invalid_argument{
            "JobGgufKv byte storage is not divisible by the requested GGUF type size"
        };
    }

    /*
     * A scalar must remain exactly one scalar after reinterpretation.
     * Arrays may change their apparent element count, matching upstream
     * gguf_kv::cast() behavior.
     */
    if (!m_isArray &&
        m_data.size() != newTypeSize) {
        throw std::invalid_argument{
            "JobGgufKv scalar storage does not contain exactly one value of the requested type"
        };
    }

    m_type = type;
}

void JobGgufKv::cast(enum gguf_type type)
{
    if (!isValidGgufType(type)) {
        throw std::invalid_argument{
            "JobGgufKv received an invalid gguf_type"
        };
    }

    cast(fromGgufType(type));
}

const std::vector<std::byte> &JobGgufKv::data() const noexcept
{
    return m_data;
}

const std::vector<std::string> &JobGgufKv::stringData() const noexcept
{
    return m_stringData;
}

void JobGgufKv::reset()
{
    m_key.clear();
    m_isArray = false;
    m_type    = JobGgufType::UInt8;

    clearValueStorage();
}

void JobGgufKv::validateKey() const
{
    if (m_key.empty()) {
        throw std::invalid_argument{
            "JobGgufKv requires a non-empty key"
        };
    }
}

void JobGgufKv::clearValueStorage() noexcept
{
    m_data.clear();
    m_stringData.clear();
}

} // namespace job::ggml