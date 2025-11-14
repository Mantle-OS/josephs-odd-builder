#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "job_serializer_utils.h"

namespace job::serializer {

// Represents a schema instance in memory (values of all fields)
class RuntimeObject final {
public:
    RuntimeObject() = default;
    ~RuntimeObject() = default;

    RuntimeObject(const RuntimeObject &) = default;
    RuntimeObject &operator=(const RuntimeObject &) = default;

    [[nodiscard]] bool hasField(const std::string &name) const noexcept;
    [[nodiscard]] std::optional<FieldValue> getField(const std::string &name) const noexcept;

    bool setField(const std::string &name, const FieldValue &val) noexcept;
    bool removeField(const std::string &name) noexcept;

    void clear() noexcept;

    [[nodiscard]] const std::unordered_map<std::string, FieldValue> &fields() const noexcept;
    [[nodiscard]] std::unordered_map<std::string, FieldValue> &fields() noexcept;

private:
    std::unordered_map<std::string, FieldValue> m_fields;
};

} // namespace job::serializer
// CHECKPOINT: v1
