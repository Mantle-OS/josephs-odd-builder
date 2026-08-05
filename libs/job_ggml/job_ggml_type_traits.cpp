#include "job_ggml_type_traits.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlTypeTraits::JobGgmlTypeTraits(JobGgmlType type)
{
    setType(type);
}

JobGgmlTypeTraits::JobGgmlTypeTraits(enum ggml_type type)
{
    setGgmlType(type);
}

JobGgmlTypeTraits::JobGgmlTypeTraits(const struct ggml_type_traits *typeTraits, enum ggml_type type)
{
    setTypeTraits(typeTraits, type);
}

bool JobGgmlTypeTraits::isValid() const noexcept
{
    return m_typeTraits != nullptr;
}

JobGgmlType JobGgmlTypeTraits::type() const noexcept
{
    return m_type;
}

enum ggml_type JobGgmlTypeTraits::ggmlType() const noexcept
{
    return m_ggmlType;
}

void JobGgmlTypeTraits::setType(JobGgmlType type)
{
    setGgmlType(toGgmlType(type));
}

void JobGgmlTypeTraits::setGgmlType(enum ggml_type type)
{
    const struct ggml_type_traits *traits = ggml_get_type_traits(type);

    if (!traits) {
        throw std::invalid_argument{
            "JobGgmlTypeTraits received an unsupported ggml_type"
        };
    }

    setTypeTraits(traits, type);
}

const std::string &JobGgmlTypeTraits::typeName() const noexcept
{
    return m_typeName;
}

std::int64_t JobGgmlTypeTraits::blockSize() const noexcept
{
    return m_blockSize;
}

std::int64_t JobGgmlTypeTraits::blockSizeInterleave() const noexcept
{
    return m_blockSizeInterleave;
}

std::size_t JobGgmlTypeTraits::typeSize() const noexcept
{
    return m_typeSize;
}

bool JobGgmlTypeTraits::isQuantized() const noexcept
{
    return m_isQuantized;
}

ggml_to_float_t JobGgmlTypeTraits::toFloatFunction() const noexcept
{
    return m_toFloat;
}

ggml_from_float_t JobGgmlTypeTraits::fromFloatReferenceFunction() const noexcept
{
    return m_fromFloatRef;
}

bool JobGgmlTypeTraits::canConvertToFloat() const noexcept
{
    return m_toFloat != nullptr;
}

bool JobGgmlTypeTraits::canConvertFromFloat() const noexcept
{
    return m_fromFloatRef != nullptr;
}

void JobGgmlTypeTraits::convertToFloat(const void *source, float *destination, std::int64_t elementCount) const
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot convert with invalid GGML type traits"
        };
    }

    if (!source) {
        throw std::invalid_argument{
            "convertToFloat requires a valid source buffer"
        };
    }

    if (!destination) {
        throw std::invalid_argument{
            "convertToFloat requires a valid destination buffer"
        };
    }

    if (elementCount <= 0) {
        throw std::invalid_argument{
            "convertToFloat requires a positive element count"
        };
    }

    if (!m_toFloat) {
        throw std::runtime_error{
            "This GGML type does not provide a to-float conversion callback"
        };
    }

    m_toFloat(source, destination, elementCount);
}

void JobGgmlTypeTraits::convertFromFloatReference(const float *source, void *destination, std::int64_t elementCount) const
{
    if (!isValid()) {
        throw std::runtime_error{
            "Cannot convert with invalid GGML type traits"
        };
    }

    if (!source) {
        throw std::invalid_argument{
            "convertFromFloatReference requires a valid source buffer"
        };
    }

    if (!destination) {
        throw std::invalid_argument{
            "convertFromFloatReference requires a valid destination buffer"
        };
    }

    if (elementCount <= 0) {
        throw std::invalid_argument{
            "convertFromFloatReference requires a positive element count"
        };
    }

    if (!m_fromFloatRef) {
        throw std::runtime_error{
            "This GGML type does not provide a reference from-float conversion callback"
        };
    }

    m_fromFloatRef(source, destination, elementCount);
}

void JobGgmlTypeTraits::setTypeTraits(const struct ggml_type_traits *typeTraits, enum ggml_type type)
{
    if (!typeTraits) {
        throw std::invalid_argument{
            "JobGgmlTypeTraits requires valid ggml_type_traits"
        };
    }

    m_typeTraits = typeTraits;
    m_ggmlType   = type;
    m_type       = fromGgmlType(type);

    fillTypeTraits();
}

const struct ggml_type_traits *JobGgmlTypeTraits::typeTraits() const noexcept
{
    return m_typeTraits;
}

void JobGgmlTypeTraits::resetTypeTraits()
{
    clearTypeTraits();
    setGgmlType(GGML_TYPE_F32);
}

void JobGgmlTypeTraits::fillTypeTraits()
{
    if (!m_typeTraits) {
        clearTypeTraits();
        return;
    }

    m_typeName              = m_typeTraits->type_name ? m_typeTraits->type_name : "unknown";
    m_blockSize             = m_typeTraits->blck_size;
    m_blockSizeInterleave   = m_typeTraits->blck_size_interleave;
    m_typeSize              = m_typeTraits->type_size;
    m_isQuantized           = m_typeTraits->is_quantized;
    m_toFloat               = m_typeTraits->to_float;
    m_fromFloatRef          = m_typeTraits->from_float_ref;
}

void JobGgmlTypeTraits::clearTypeTraits() noexcept
{
    m_typeTraits            = nullptr;
    m_ggmlType              = GGML_TYPE_F32;
    m_type                  = JobGgmlType::F32;
    m_typeName              = "f32";
    m_blockSize             = 0;
    m_blockSizeInterleave   = 0;
    m_typeSize              = 0;
    m_isQuantized           = false;
    m_toFloat               = nullptr;
    m_fromFloatRef          = nullptr;
}

} // namespace job::ggml