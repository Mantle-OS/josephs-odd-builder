#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "job_drm_property.h"

namespace job::render {

class JobDrmAtomicState
{
public:
    using Ptr  = std::shared_ptr<JobDrmAtomicState>;
    using WPtr = std::weak_ptr<JobDrmAtomicState>;
    using UPtr = std::unique_ptr<JobDrmAtomicState>;

    using ObjectId      = std::uint32_t;
    using ObjectType    = std::uint32_t;
    using PropertyId    = JobDrmProperty::Id;
    using Value         = JobDrmProperty::Value;

    struct Property
    {
        PropertyId  id{};
        Value       value{};

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return id != 0;
        }

        [[nodiscard]] friend constexpr bool operator==(
            const Property &,
            const Property &) = default;
    };

    using Properties = std::vector<Property>;
    struct Object
    {
        ObjectId    id{};
        ObjectType  type{};
        Properties  properties;

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return id != 0 && type != 0;
        }

        [[nodiscard]] const Property *find(PropertyId propertyId) const noexcept
        {
            for (const auto &property : properties) {
                if (property.id == propertyId)
                    return &property;
            }

            return nullptr;
        }

        [[nodiscard]] Property *find(PropertyId propertyId) noexcept
        {
            for (auto &property : properties) {
                if (property.id == propertyId)
                    return &property;
            }

            return nullptr;
        }

        [[nodiscard]] friend bool operator==(const Object &, const Object &) = default;
    };

    using Objects = std::vector<Object>;

    JobDrmAtomicState() = default;
    ~JobDrmAtomicState() = default;

    JobDrmAtomicState(const JobDrmAtomicState &) = default;
    JobDrmAtomicState &operator=(const JobDrmAtomicState &) = default;

    JobDrmAtomicState(JobDrmAtomicState &&) noexcept = default;
    JobDrmAtomicState &operator=(JobDrmAtomicState &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmAtomicState>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmAtomicState>();
    }

    [[nodiscard]] std::span<const Object> objects() const noexcept
    {
        return m_objects;
    }

    [[nodiscard]] std::size_t objectCount() const noexcept
    {
        return m_objects.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_objects.empty();
    }

    [[nodiscard]] const Object *findObject(ObjectId objectId) const noexcept
    {
        for (const auto &object : m_objects) {
            if (object.id == objectId)
                return &object;
        }

        return nullptr;
    }

    [[nodiscard]] Object *findObject(ObjectId objectId) noexcept
    {
        for (auto &object : m_objects) {
            if (object.id == objectId)
                return &object;
        }

        return nullptr;
    }

    [[nodiscard]] const Property *findProperty(ObjectId objectId, PropertyId propertyId) const noexcept
    {
        const Object *object = findObject(objectId);
        if (!object)
            return nullptr;

        return object->find(propertyId);
    }

    [[nodiscard]] bool set(ObjectId objectId, ObjectType objectType, PropertyId propertyId, Value value)
    {
        if (objectId == 0 || objectType == 0 || propertyId == 0)
            return false;

        Object *object = findObject(objectId);
        if (!object) {
            m_objects.push_back(Object{
                .id = objectId,
                .type = objectType,
                .properties = {}
            });

            object = &m_objects.back();
        } else if (object->type != objectType) {
            return false;
        }

        Property *property = object->find(propertyId);
        if (property) {
            property->value = value;
            return true;
        }

        object->properties.push_back(Property{
            .id = propertyId,
            .value = value
        });

        return true;
    }

    [[nodiscard]] bool remove(ObjectId objectId, PropertyId propertyId) noexcept
    {
        Object *object = findObject(objectId);

        if (!object)
            return false;

        for (auto it = object->properties.begin(); it != object->properties.end(); ++it) {
            if (it->id != propertyId)
                continue;

            object->properties.erase(it);
            if (object->properties.empty()) {
                if (!removeObject(objectId))
                    return false;
            }

            return true;
        }

        return false;
    }

    [[nodiscard]] bool removeObject(ObjectId objectId) noexcept
    {
        for (auto it = m_objects.begin();
             it != m_objects.end();
             ++it) {
            if (it->id != objectId)
                continue;

            m_objects.erase(it);
            return true;
        }

        return false;
    }

    void clear() noexcept
    {
        m_objects.clear();
    }

    [[nodiscard]] bool valid() const noexcept
    {
        for (const auto &object : m_objects) {
            if (!object.valid())
                return false;

            for (const auto &property : object.properties) {
                if (!property.valid())
                    return false;
            }
        }

        return true;
    }

    [[nodiscard]] friend bool operator==(const JobDrmAtomicState &, const JobDrmAtomicState &) = default;

private:
    Objects m_objects;
};

}