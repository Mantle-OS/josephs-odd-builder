#include "job_gguf_type_traits.h"

#include <stdexcept>

namespace job::ggml {

JobGgufTypeTraits::JobGgufTypeTraits(JobGgufType type)
{
    setType(type);
}

JobGgufTypeTraits::JobGgufTypeTraits(enum gguf_type type)
{
    setGgufType(type);
}

bool JobGgufTypeTraits::isValid() const noexcept
{
    return isValidGgufType(m_type);
}

JobGgufType JobGgufTypeTraits::type() const noexcept
{
    return m_type;
}

enum gguf_type JobGgufTypeTraits::ggufType() const noexcept
{
    return toGgufType(m_type);
}

void JobGgufTypeTraits::setType(JobGgufType type)
{
    if (!isValidGgufType(type)) {
        throw std::invalid_argument{
            "JobGgufTypeTraits received an invalid JobGgufType"
        };
    }

    m_type = type;
}

void JobGgufTypeTraits::setGgufType(enum gguf_type type)
{
    if (!isValidGgufType(type)) {
        throw std::invalid_argument{
            "JobGgufTypeTraits received an invalid gguf_type"
        };
    }

    setType(fromGgufType(type));
}

std::string_view JobGgufTypeTraits::typeName() const noexcept
{
    return ggufTypeName(m_type);
}

std::size_t JobGgufTypeTraits::typeSize() const noexcept
{
    return ggufTypeSize(m_type);
}

bool JobGgufTypeTraits::hasFixedSize() const noexcept
{
    return typeSize() > 0;
}

bool JobGgufTypeTraits::isInteger() const noexcept
{
    return isSignedInteger() || isUnsignedInteger();
}

bool JobGgufTypeTraits::isSignedInteger() const noexcept
{
    switch (m_type) {
    case JobGgufType::Int8:
    case JobGgufType::Int16:
    case JobGgufType::Int32:
    case JobGgufType::Int64:
        return true;

    default:
        return false;
    }
}

bool JobGgufTypeTraits::isUnsignedInteger() const noexcept
{
    switch (m_type) {
    case JobGgufType::UInt8:
    case JobGgufType::UInt16:
    case JobGgufType::UInt32:
    case JobGgufType::UInt64:
        return true;

    default:
        return false;
    }
}

bool JobGgufTypeTraits::isFloatingPoint() const noexcept
{
    return m_type == JobGgufType::Float32 || m_type == JobGgufType::Float64;
}

bool JobGgufTypeTraits::isBoolean() const noexcept
{
    return m_type == JobGgufType::Bool;
}

bool JobGgufTypeTraits::isString() const noexcept
{
    return m_type == JobGgufType::String;
}

bool JobGgufTypeTraits::isArray() const noexcept
{
    return m_type == JobGgufType::Array;
}

bool JobGgufTypeTraits::isValueType() const noexcept
{
    return isValid() && !isArray();
}

bool JobGgufTypeTraits::isArrayElementType() const noexcept
{
    /*
     * GGUF arrays may contain scalar numeric values, booleans, or strings.
     * Nested arrays are not valid GGUF array element types.
     */
    return isValueType();
}

} // namespace job::ggml