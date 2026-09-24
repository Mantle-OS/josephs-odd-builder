#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <drm_mode.h>
#include <xf86drmMode.h>

namespace job::render {

enum class DrmPropertyType : std::uint8_t
{
    Unknown,
    Range,
    SignedRange,
    Enum,
    Bitmask,
    Object,
    Blob
};

class JobDrmProperty
{
public:
    struct EnumValue
    {
        std::uint64_t   value{};
        std::string     name;
        [[nodiscard]] friend bool operator==(const EnumValue &, const EnumValue &) = default;
    };

    using Ptr  = std::shared_ptr<JobDrmProperty>;
    using WPtr = std::weak_ptr<JobDrmProperty>;
    using UPtr = std::unique_ptr<JobDrmProperty>;

    using Id            = std::uint32_t;
    using Flags         = std::uint32_t;
    using Value         = std::uint64_t;
    using Values        = std::vector<Value>;
    using EnumValues    = std::vector<EnumValue>;

    JobDrmProperty() = default;

    JobDrmProperty(Id id, Flags flags, std::string name, Values values, EnumValues enumValues) :
        m_id(id),
        m_flags(flags),
        m_name(std::move(name)),
        m_values(std::move(values)),
        m_enumValues(std::move(enumValues))
    {
    }

    ~JobDrmProperty() = default;

    JobDrmProperty(const JobDrmProperty &) = default;
    JobDrmProperty &operator=(const JobDrmProperty &) = default;

    JobDrmProperty(JobDrmProperty &&) noexcept = default;
    JobDrmProperty &operator=(JobDrmProperty &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id, Flags flags, std::string name, Values values, EnumValues enumValues)
    {
        return std::make_shared<JobDrmProperty>(id,
                                                flags,
                                                std::move(name),
                                                std::move(values),
                                                std::move(enumValues));
    }

    [[nodiscard]] static UPtr createUniq(Id id, Flags flags, std::string name, Values values, EnumValues enumValues)
    {
        return std::make_unique<JobDrmProperty>(id,
                                                flags,
                                                std::move(name),
                                                std::move(values),
                                                std::move(enumValues));
    }

    [[nodiscard]] static JobDrmProperty fromDrm(const drmModePropertyRes &property)
    {
        Values values;
        values.reserve(property.count_values);

        for (int i = 0; i < property.count_values; ++i)
            values.push_back(property.values[i]);

        EnumValues enumValues;
        enumValues.reserve(property.count_enums);

        for (int i = 0; i < property.count_enums; ++i) {
            enumValues.push_back({
                .value = property.enums[i].value,
                .name = property.enums[i].name
            });
        }

        return JobDrmProperty{
            property.prop_id,
            property.flags,
            property.name,
            std::move(values),
            std::move(enumValues)
        };
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr Flags flags() const noexcept
    {
        return m_flags;
    }

    [[nodiscard]] std::string_view name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] const Values &values() const noexcept
    {
        return m_values;
    }

    [[nodiscard]] const EnumValues &enumValues() const noexcept
    {
        return m_enumValues;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] constexpr bool immutable() const noexcept
    {
        return (m_flags & DRM_MODE_PROP_IMMUTABLE) != 0;
    }

    [[nodiscard]] constexpr bool atomic() const noexcept
    {
        return (m_flags & DRM_MODE_PROP_ATOMIC) != 0;
    }

    [[nodiscard]] constexpr DrmPropertyType type() const noexcept
    {
        if ((m_flags & DRM_MODE_PROP_RANGE) != 0)
            return DrmPropertyType::Range;

        if ((m_flags & DRM_MODE_PROP_SIGNED_RANGE) != 0)
            return DrmPropertyType::SignedRange;

        if ((m_flags & DRM_MODE_PROP_ENUM) != 0)
            return DrmPropertyType::Enum;

        if ((m_flags & DRM_MODE_PROP_BITMASK) != 0)
            return DrmPropertyType::Bitmask;

        if ((m_flags & DRM_MODE_PROP_OBJECT) != 0)
            return DrmPropertyType::Object;

        if ((m_flags & DRM_MODE_PROP_BLOB) != 0)
            return DrmPropertyType::Blob;

        return DrmPropertyType::Unknown;
    }

    [[nodiscard]] constexpr bool range() const noexcept
    {
        return type() == DrmPropertyType::Range;
    }

    [[nodiscard]] constexpr bool signedRange() const noexcept
    {
        return type() == DrmPropertyType::SignedRange;
    }

    [[nodiscard]] constexpr bool enumeration() const noexcept
    {
        return type() == DrmPropertyType::Enum;
    }

    [[nodiscard]] constexpr bool bitmask() const noexcept
    {
        return type() == DrmPropertyType::Bitmask;
    }

    [[nodiscard]] constexpr bool object() const noexcept
    {
        return type() == DrmPropertyType::Object;
    }

    [[nodiscard]] constexpr bool blob() const noexcept
    {
        return type() == DrmPropertyType::Blob;
    }

    [[nodiscard]] constexpr bool hasFlag(Flags flag) const noexcept
    {
        return (m_flags & flag) == flag;
    }

    [[nodiscard]] const EnumValue *enumValue(Value value) const noexcept
    {
        for (const auto &entry : m_enumValues) {
            if (entry.value == value)
                return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] const EnumValue *enumValue(std::string_view name) const noexcept
    {
        for (const auto &entry : m_enumValues) {
            if (entry.name == name)
                return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] constexpr Value minimum() const noexcept
    {
        if (!range() || m_values.size() < 2)
            return 0;

        return m_values[0];
    }

    [[nodiscard]] constexpr Value maximum() const noexcept
    {
        if (!range() || m_values.size() < 2)
            return 0;

        return m_values[1];
    }

    [[nodiscard]] constexpr std::int64_t signedMinimum() const noexcept
    {
        if (!signedRange() || m_values.size() < 2)
            return 0;

        return static_cast<std::int64_t>(m_values[0]);
    }

    [[nodiscard]] constexpr std::int64_t signedMaximum() const noexcept
    {
        if (!signedRange() || m_values.size() < 2)
            return 0;

        return static_cast<std::int64_t>(m_values[1]);
    }

    [[nodiscard]] constexpr std::uint32_t objectType() const noexcept
    {
        if (!object() || m_values.empty())
            return 0;

        return static_cast<std::uint32_t>(m_values[0]);
    }

    [[nodiscard]] friend bool operator==(const JobDrmProperty &, const JobDrmProperty &) = default;

private:
    Id              m_id{};
    Flags           m_flags{};
    std::string     m_name;
    Values          m_values;
    EnumValues      m_enumValues;
};

}