#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include <gguf.h>

#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

template<typename T>
struct JobGgufTypeMap;

template<>
struct JobGgufTypeMap<std::uint8_t> { static constexpr JobGgufType type = JobGgufType::UInt8; };

template<>
struct JobGgufTypeMap<std::int8_t> { static constexpr JobGgufType type = JobGgufType::Int8; };

template<>
struct JobGgufTypeMap<std::uint16_t> { static constexpr JobGgufType type = JobGgufType::UInt16; };

template<>
struct JobGgufTypeMap<std::int16_t> { static constexpr JobGgufType type = JobGgufType::Int16; };

template<>
struct JobGgufTypeMap<std::uint32_t> { static constexpr JobGgufType type = JobGgufType::UInt32; };

template<>
struct JobGgufTypeMap<std::int32_t> { static constexpr JobGgufType type = JobGgufType::Int32; };

template<>
struct JobGgufTypeMap<float> { static constexpr JobGgufType type = JobGgufType::Float32; };

template<>
struct JobGgufTypeMap<bool> { static constexpr JobGgufType type = JobGgufType::Bool; };

template<>
struct JobGgufTypeMap<std::string> { static constexpr JobGgufType type = JobGgufType::String; };

template<>
struct JobGgufTypeMap<std::uint64_t> { static constexpr JobGgufType type = JobGgufType::UInt64; };

template<>
struct JobGgufTypeMap<std::int64_t> { static constexpr JobGgufType type = JobGgufType::Int64; };

template<>
struct JobGgufTypeMap<double> { static constexpr JobGgufType type = JobGgufType::Float64; };

/*
 * True when T has a supported scalar GGUF representation.
 *
 * Array is not mapped from a C++ value type here. GGUF_TYPE_ARRAY describes
 * the container; its element type remains one of the mapped scalar types.
 */
template<typename T, typename = void>
struct IsJobGgufValueType : std::false_type {};

template<typename T>
struct IsJobGgufValueType<T, std::void_t<decltype(JobGgufTypeMap<std::remove_cvref_t<T>>::type)>> : std::true_type { };

template<typename T>
inline constexpr bool IsJobGgufValueTypeV = IsJobGgufValueType<T>::value;

template<typename T>
concept JobGgufValueType = IsJobGgufValueTypeV<T>;

class JOBGGML_EXPORT JobGgufTypeTraits
{
public:
    using Ptr  = std::shared_ptr<JobGgufTypeTraits>;
    using WPtr = std::weak_ptr<JobGgufTypeTraits>;
    using UPtr = std::unique_ptr<JobGgufTypeTraits>;

    explicit JobGgufTypeTraits(JobGgufType type = JobGgufType::UInt8);
    explicit JobGgufTypeTraits(enum gguf_type type);

    ~JobGgufTypeTraits() = default;

    [[nodiscard]] static Ptr createShared(JobGgufType type = JobGgufType::UInt8)
    {
        return std::make_shared<JobGgufTypeTraits>(type);
    }

    [[nodiscard]] static Ptr createShared(enum gguf_type type)
    {
        return std::make_shared<JobGgufTypeTraits>(type);
    }

    [[nodiscard]] static UPtr createUniq(JobGgufType type = JobGgufType::UInt8)
    {
        return std::make_unique<JobGgufTypeTraits>(type);
    }

    [[nodiscard]] static UPtr createUniq(enum gguf_type type)
    {
        return std::make_unique<JobGgufTypeTraits>(type);
    }

    JobGgufTypeTraits(const JobGgufTypeTraits &) = delete;
    JobGgufTypeTraits &operator=(const JobGgufTypeTraits &) = delete;
    JobGgufTypeTraits(JobGgufTypeTraits &&) = delete;
    JobGgufTypeTraits &operator=(JobGgufTypeTraits &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] JobGgufType type() const noexcept;
    [[nodiscard]] enum gguf_type ggufType() const noexcept;

    void setType(JobGgufType type);
    void setGgufType(enum gguf_type type);

    [[nodiscard]] std::string_view typeName() const noexcept;

    [[nodiscard]] std::size_t typeSize() const noexcept;
    [[nodiscard]] bool hasFixedSize() const noexcept;

    [[nodiscard]] bool isInteger() const noexcept;
    [[nodiscard]] bool isSignedInteger() const noexcept;
    [[nodiscard]] bool isUnsignedInteger() const noexcept;
    [[nodiscard]] bool isFloatingPoint() const noexcept;

    [[nodiscard]] bool isBoolean() const noexcept;
    [[nodiscard]] bool isString() const noexcept;
    [[nodiscard]] bool isArray() const noexcept;

    /*
     * True for types that can be stored as a scalar value or used as an
     * array's element type.
     */
    [[nodiscard]] bool isValueType() const noexcept;
    [[nodiscard]] bool isArrayElementType() const noexcept;

    template<JobGgufValueType T>
    [[nodiscard]] static constexpr JobGgufType typeFor() noexcept
    {
        return JobGgufTypeMap<std::remove_cvref_t<T>>::type;
    }

    template<JobGgufValueType T>
    [[nodiscard]] static constexpr enum gguf_type ggufTypeFor() noexcept
    {
        return toGgufType(typeFor<T>());
    }

    template<typename T>
    [[nodiscard]] static constexpr bool supportsType() noexcept
    {
        return IsJobGgufValueTypeV<T>;
    }

    template<JobGgufValueType T>
    [[nodiscard]] bool isType() const noexcept
    {
        return m_type == typeFor<T>();
    }

    [[nodiscard]] static constexpr JobGgufType fromGgufType(enum gguf_type type) noexcept
    {
        return job::ggml::fromGgufType(type);
    }

    [[nodiscard]] static constexpr enum gguf_type toGgufType(JobGgufType type) noexcept
    {
        return job::ggml::toGgufType(type);
    }

private:
    JobGgufType m_type{JobGgufType::UInt8};
};

} // namespace job::ggml