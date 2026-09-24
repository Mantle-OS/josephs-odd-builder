#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "job_drm_property.h"

namespace job::render {

class JobDrmObjectProperties
{
public:
    using Ptr  = std::shared_ptr<JobDrmObjectProperties>;
    using WPtr = std::weak_ptr<JobDrmObjectProperties>;
    using UPtr = std::unique_ptr<JobDrmObjectProperties>;

    using ObjectId      = std::uint32_t;
    using ObjectType    = std::uint32_t;
    using PropertyId    = JobDrmProperty::Id;
    using Value         = JobDrmProperty::Value;

    struct Entry
    {
        JobDrmProperty  property;
        Value           value{};
        [[nodiscard]] constexpr PropertyId id() const noexcept
        {
            return property.id();
        }

        [[nodiscard]] std::string_view name() const noexcept
        {
            return property.name();
        }

        [[nodiscard]] friend bool operator==(const Entry &, const Entry &) = default;
    };


    using Entries = std::vector<Entry>;
    JobDrmObjectProperties() = default;

    JobDrmObjectProperties(ObjectId objectId, ObjectType objectType, Entries entries) :
        m_objectId(objectId),
        m_objectType(objectType),
        m_entries(std::move(entries))
    {
    }

    ~JobDrmObjectProperties() = default;

    JobDrmObjectProperties(const JobDrmObjectProperties &) = default;
    JobDrmObjectProperties &operator=(const JobDrmObjectProperties &) = default;

    JobDrmObjectProperties(JobDrmObjectProperties &&) noexcept = default;
    JobDrmObjectProperties &operator=(JobDrmObjectProperties &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(ObjectId objectId, ObjectType objectType, Entries entries)
    {
        return std::make_shared<JobDrmObjectProperties>(objectId, objectType, std::move(entries));
    }

    [[nodiscard]] static UPtr createUniq(ObjectId objectId, ObjectType objectType, Entries entries)
    {
        return std::make_unique<JobDrmObjectProperties>(objectId, objectType, std::move(entries));
    }

    [[nodiscard]] constexpr ObjectId objectId() const noexcept
    {
        return m_objectId;
    }

    [[nodiscard]] constexpr ObjectType objectType() const noexcept
    {
        return m_objectType;
    }

    [[nodiscard]] std::span<const Entry> entries() const noexcept
    {
        return m_entries;
    }

    [[nodiscard]] std::size_t count() const noexcept
    {
        return m_entries.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_entries.empty();
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_objectId != 0 && m_objectType != 0;
    }

    [[nodiscard]] const Entry *find(PropertyId id) const noexcept
    {
        for (const auto &entry : m_entries) {
            if (entry.id() == id)
                return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] const Entry *find(std::string_view name) const noexcept
    {
        for (const auto &entry : m_entries) {
            if (entry.name() == name)
                return &entry;
        }

        return nullptr;
    }

    [[nodiscard]] bool contains(PropertyId id) const noexcept
    {
        return find(id) != nullptr;
    }

    [[nodiscard]] bool contains(std::string_view name) const noexcept
    {
        return find(name) != nullptr;
    }

    [[nodiscard]] const JobDrmProperty *property(PropertyId id) const noexcept
    {
        const auto *entry = find(id);
        return entry ? &entry->property : nullptr;
    }

    [[nodiscard]] const JobDrmProperty *property(std::string_view name) const noexcept
    {
        const auto *entry = find(name);
        return entry ? &entry->property : nullptr;
    }

    [[nodiscard]] std::optional<Value> value(PropertyId id) const noexcept
    {
        const auto *entry = find(id);
        if (!entry)
            return std::nullopt;

        return entry->value;
    }

    [[nodiscard]] std::optional<Value> value(std::string_view name) const noexcept
    {
        const auto *entry = find(name);
        if (!entry)
            return std::nullopt;

        return entry->value;
    }

    [[nodiscard]] friend bool operator==(const JobDrmObjectProperties &, const JobDrmObjectProperties &) = default;

private:
    ObjectId    m_objectId{};
    ObjectType  m_objectType{};
    Entries     m_entries;
};

}