#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "job_serializer_utils.h"
#include "jobserializer_export.h"

namespace job::serializer {

// Represents a schema instance in memory (values of all fields)
class JOBSERIALIZER_EXPORT RuntimeObject final
{
public:
    using Ptr       = std::shared_ptr<RuntimeObject>;
    using WPtr      = std::weak_ptr<RuntimeObject>;
    using UPtr      = std::unique_ptr<RuntimeObject>;
    using Fields    = std::unordered_map<std::string, FieldValue>;
    RuntimeObject() = default;
    ~RuntimeObject() = default;

    RuntimeObject(const RuntimeObject &) = default;
    RuntimeObject &operator=(const RuntimeObject &) = default;
    RuntimeObject(RuntimeObject &&) noexcept = default;
    RuntimeObject &operator=(RuntimeObject &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<RuntimeObject>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<RuntimeObject>();
    }

    [[nodiscard]] bool hasField(const std::string &name) const noexcept;
    [[nodiscard]] std::optional<FieldValue> getField(const std::string &name) const noexcept;

    bool setField(const std::string &name, const FieldValue &val) noexcept;
    bool removeField(const std::string &name) noexcept;

    void clear() noexcept;

    [[nodiscard]] const Fields &fields() const noexcept;
    [[nodiscard]] Fields &fields() noexcept;

private:
    Fields m_fields;
};

} // namespace job::serializer