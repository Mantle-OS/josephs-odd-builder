#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "ser_obj_concept.h"
#include "ser_nested_obj.h"

namespace job::model {

class SerObj
{
public:
    using Ptr  = std::shared_ptr<SerObj>;
    using WPtr = std::weak_ptr<SerObj>;
    using UPtr = std::unique_ptr<SerObj>;

    SerObj() = default;
    ~SerObj() = default;

    SerObj(const SerObj &) = delete;
    SerObj &operator=(const SerObj &) = delete;
    SerObj(SerObj &&) noexcept = default;
    SerObj &operator=(SerObj &&) noexcept = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<SerObj>(); }
    [[nodiscard]] static UPtr createUniq() { return std::make_unique<SerObj>(); }

    [[nodiscard]] int count() const noexcept { return m_count; }
    void setCount(int count) noexcept { m_count = count; }

    [[nodiscard]] std::vector<float> &floatList() noexcept { return m_floatList; }
    [[nodiscard]] const std::vector<float> &floatList() const noexcept { return m_floatList; }
    void setFloatList(std::vector<float> value) { m_floatList = std::move(value); }

    [[nodiscard]] std::vector<SerNestedObj::Ptr> &nestedObjects() noexcept { return m_nestedObjects; }
    [[nodiscard]] const std::vector<SerNestedObj::Ptr> &nestedObjects() const noexcept { return m_nestedObjects; }
    void setNestedObjects(std::vector<SerNestedObj::Ptr> value) { m_nestedObjects = std::move(value); }

    [[nodiscard]] float value() const noexcept { return m_value; }
    void setValue(float value) noexcept { m_value = value; }

    [[nodiscard]] const std::string &name() const noexcept { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    [[nodiscard]] SerNestedObj &nestedObject() noexcept { return m_nestedObject; }
    [[nodiscard]] const SerNestedObj &nestedObject() const noexcept { return m_nestedObject; }

    [[nodiscard]] std::vector<std::byte> toBinary() const
    {
        std::vector<std::byte> out;

        template for (constexpr auto member : serMembers<SerObj>())
            writeValue(out, this->[:member:]);

        return out;
    }

    bool fromBinary(std::span<const std::byte> data)
    {
        std::size_t offset = 0;

        template for (constexpr auto member : serMembers<SerObj>()) {
            if (!readValue(data, offset, this->[:member:]))
                return false;
        }

        return offset == data.size();
    }

private:
    template <typename T>
    static void writePod(std::vector<std::byte> &out, const T &value)
    {
        const auto oldSize = out.size();
        out.resize(oldSize + sizeof(T));
        std::memcpy(out.data() + oldSize, &value, sizeof(T));
    }

    template <typename T>
    static bool readPod(std::span<const std::byte> data, std::size_t &offset, T &value)
    {
        if (offset + sizeof(T) > data.size())
            return false;

        std::memcpy(&value, data.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }

    template <typename T>
    static void writeValue(std::vector<std::byte> &out, const T &value)
    {
        if constexpr (std::is_arithmetic_v<T>) {
            writePod(out, value);
        } else if constexpr (std::is_enum_v<T>) {
            using U = std::underlying_type_t<T>;
            writePod(out, static_cast<U>(value));
        } else if constexpr (std::same_as<T, std::byte>) {
            writePod(out, value);
        } else if constexpr (std::same_as<T, std::string>) {
            const std::uint64_t size = value.size();
            writePod(out, size);

            const auto oldSize = out.size();
            out.resize(oldSize + size);
            std::memcpy(out.data() + oldSize, value.data(), size);
        } else if constexpr (requires { typename T::value_type; value.size(); value.begin(); }) {
            const std::uint64_t size = value.size();
            writePod(out, size);

            for (const auto &item : value) {
                if constexpr (requires { item.get(); }) {
                    const bool present = static_cast<bool>(item);
                    writePod(out, present);
                    if (present)
                        writeValue(out, *item);
                } else {
                    writeValue(out, item);
                }
            }
        } else if constexpr (SerObject<T>) {
            template for (constexpr auto member : serMembers<T>())
            writeValue(out, value.[:member:]);
        } else {
            static_assert(false, "Unsupported serialization type");
        }
    }

    template <typename T>
    static bool readValue(std::span<const std::byte> data, std::size_t &offset, T &value)
    {
        if constexpr (std::is_arithmetic_v<T>) {
            return readPod(data, offset, value);
        } else if constexpr (std::is_enum_v<T>) {
            using U = std::underlying_type_t<T>;
            U raw{};

            if (!readPod(data, offset, raw))
                return false;

            value = static_cast<T>(raw);
            return true;
        } else if constexpr (std::same_as<T, std::byte>) {
            return readPod(data, offset, value);
        } else if constexpr (std::same_as<T, std::string>) {
            std::uint64_t size = 0;

            if (!readPod(data, offset, size) || offset + size > data.size())
                return false;

            value.assign(reinterpret_cast<const char *>(data.data() + offset), size);
            offset += size;
            return true;
        } else if constexpr (requires { typename T::value_type; value.resize(std::size_t{}); }) {
            std::uint64_t size = 0;

            if (!readPod(data, offset, size))
                return false;

            value.clear();
            value.reserve(size);

            for (std::uint64_t i = 0; i < size; ++i) {
                using V = typename T::value_type;

                if constexpr (requires(V v) { v.get(); }) {
                    bool present = false;

                    if (!readPod(data, offset, present))
                        return false;

                    if (!present) {
                        value.push_back({});
                        continue;
                    }

                    using P = typename V::element_type;
                    auto item = P::createShared();

                    if (!readValue(data, offset, *item))
                        return false;

                    value.push_back(std::move(item));
                } else {
                    V item{};

                    if (!readValue(data, offset, item))
                        return false;

                    value.push_back(std::move(item));
                }
            }

            return true;
        } else if constexpr (SerObject<T>) {
            template for (constexpr auto member : serMembers<T>()) {
                if (!readValue(data, offset, value.[:member:]))
                    return false;
            }

            return true;
        } else {
            static_assert(false, "Unsupported serialization type");
        }
    }

    int m_count{0};
    std::vector<float> m_floatList;
    std::vector<SerNestedObj::Ptr> m_nestedObjects;
    float m_value{0.0f};
    std::string m_name;
    SerNestedObj m_nestedObject;
};

static_assert(SerObject<SerObj>);

} // namespace job::model