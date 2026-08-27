#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "obj.h"
#include "obj_concept.h"
#include "ser_nested_obj.h"

class SerObj : public Object
{
public:
    using Ptr = std::shared_ptr<SerObj>;

    SerObj() = default;
    ~SerObj() = default;

    [[nodiscard]] int count() const noexcept { return m_count; }
    void setCount(int count) noexcept { m_count = count; }

    [[nodiscard]] std::vector<float> &floatList() noexcept { return m_floatList; }
    [[nodiscard]] const std::vector<float> &floatList() const noexcept { return m_floatList; }
    void setFloatList(std::vector<float> value) { m_floatList = std::move(value); }

    [[nodiscard]] std::vector<std::shared_ptr<SerNestedObj>> &nestedObjects() noexcept { return m_nestedObjects; }
    [[nodiscard]] const std::vector<std::shared_ptr<SerNestedObj>> &nestedObjects() const noexcept { return m_nestedObjects; }
    void setNestedObjects(std::vector<std::shared_ptr<SerNestedObj>> value) { m_nestedObjects = std::move(value); }

    [[nodiscard]] float value() const noexcept { return m_value; }
    void setValue(float value) noexcept { m_value = value; }

    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    [[nodiscard]] SerNestedObj &nestedObject() noexcept { return m_nestedObject; }
    [[nodiscard]] const SerNestedObj &nestedObject() const noexcept { return m_nestedObject; }

    int m_count{0};
    std::vector<float> m_floatList;
    std::vector<std::shared_ptr<SerNestedObj>> m_nestedObjects;
    float m_value{0.0f};
    std::string m_name;
    SerNestedObj m_nestedObject;
};

static_assert(ObjectType<SerObj>);