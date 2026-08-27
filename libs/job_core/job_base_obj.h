#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "jobcore_export.h"
#include "job_obj_concept.h"

namespace job::core {

// Concept for smart pointers
template <typename T>
concept SmartPointer = requires(T p) {
    typename T::element_type;
    p.get();
    static_cast<bool>(p);
    *p;
};

// Concept for extended char types that YAML-cpp doesn't support natively
template <typename T>
concept ExtendedCharType =
    std::same_as<T, wchar_t>  ||
    std::same_as<T, char8_t>  ||
    std::same_as<T, char16_t> ||
    std::same_as<T, char32_t>;

class JOBCORE_EXPORT BaseObject
{
public:
    ~BaseObject() = default;

    std::string lastErrorString;

    // =========================================================================
    // JSON Serialization
    // =========================================================================

    template <typename Self>
    nlohmann::json toJson(this const Self& self)
    {
        nlohmann::json j = nlohmann::json::object();

        template for (constexpr auto member : reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!SignalType<MemberType>) {
                constexpr auto name = std::meta::identifier_of(member);

                if constexpr (name != "lastErrorString") {
                    j[std::string(name)] = serializeJsonValue(self.[:member:]);
                }
            }
        }

        return j;
    }

    template <typename Self>
    bool fromJson(this Self& self, const nlohmann::json& j)
    {
        try {
            template for (constexpr auto member : reflectedDataMembersV<Self>) {
                using MemberType = typename[:std::meta::type_of(member):];

                if constexpr (!SignalType<MemberType>) {
                    constexpr auto name = std::meta::identifier_of(member);

                    if constexpr (name != "lastErrorString") {
                        const std::string key(name);

                        if (j.contains(key) && !j[key].is_null()) {
                            deserializeJsonValue(j[key], self.[:member:]);
                        }
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    template <typename Self>
    void debugJson(this const Self &self, int indent = 4)
    {
        std::cout << "[JSON]\n" << self.toJson().dump(indent) << std::endl;
    }

    template <typename Self>
    bool saveToJsonFile(this const Self &self, const std::string& fileName)
    {
        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self&>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << self.toJson().dump(4);
        return true;
    }

    template <typename Self>
    bool loadFromJsonFile(this Self& self, const std::string& fileName)
    {
        std::ifstream file(fileName);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        try {
            nlohmann::json j;
            file >> j;
            return self.fromJson(j);
        } catch (const std::exception& e) {
            self.lastErrorString = e.what();
            return false;
        }
    }


    // =========================================================================
    // YAML CPP Serialization
    // =========================================================================

    template <typename Self>
    YAML::Node toYaml(this const Self& self)
    {
        YAML::Node node;

        template for (constexpr auto member : reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!SignalType<MemberType>) {
                constexpr auto name = std::meta::identifier_of(member);

                if constexpr (name != "lastErrorString") {
                    node[std::string(name)] = serializeYamlValue(self.[:member:]);
                }
            }
        }

        return node;
    }

    template <typename Self>
    bool fromYaml(this Self& self, const YAML::Node& node)
    {
        try {
            if (!node.IsMap()) {
                self.lastErrorString = "YAML node is not a map";
                return false;
            }

            template for (constexpr auto member : reflectedDataMembersV<Self>) {
                using MemberType = typename[:std::meta::type_of(member):];

                if constexpr (!SignalType<MemberType>) {
                    constexpr auto name = std::meta::identifier_of(member);

                    if constexpr (name != "lastErrorString") {
                        const std::string key(name);

                        if (node[key]) {
                            deserializeYamlValue(node[key], self.[:member:]);
                        }
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    template <typename Self>
    void debugYaml(this const Self& self)
    {
        YAML::Emitter emitter;
        emitter << self.toYaml();
        std::cout << "[YAML]\n" << emitter.c_str() << std::endl;
    }

    template <typename Self>
    bool saveToYamlFile(this const Self& self, const std::string& fileName)
    {
        YAML::Emitter out;
        out << self.toYaml();

        if (!out.good()) {
            const_cast<Self&>(self).lastErrorString = "YAML error: " + std::string(out.GetLastError());
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self&>(self).lastErrorString = "Failed opening file: " + fileName;
            return false;
        }

        file << out.c_str();
        return true;
    }

    template <typename Self>
    bool loadFromYamlFile(this Self& self, const std::string& fileName)
    {
        try {
            YAML::Node node = YAML::LoadFile(fileName);
            return self.fromYaml(node);
        } catch (const YAML::Exception& e) {
            self.lastErrorString = "Error parsing YAML file: " + std::string(e.what());
            return false;
        }
    }

    // =========================================================================
    // Binary Serialization
    // =========================================================================

    template <typename Self>
    void toBinary(this const Self& self, std::vector<uint8_t>& buffer)
    {
        template for (constexpr auto member : reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!SignalType<MemberType>) {
                constexpr auto name = std::meta::identifier_of(member);

                if constexpr (name != "lastErrorString") {
                    writeBinary(buffer, self.[:member:]);
                }
            }
        }
    }

    template <typename Self>
    bool fromBinary(this Self& self, std::span<const uint8_t>& streamSpan)
    {
        try {
            template for (constexpr auto member : reflectedDataMembersV<Self>) {
                using MemberType = typename[:std::meta::type_of(member):];

                if constexpr (!SignalType<MemberType>) {
                    constexpr auto name = std::meta::identifier_of(member);

                    if constexpr (name != "lastErrorString") {
                        readBinary(streamSpan, self.[:member:]);
                    }
                }
            }

            return true;
        } catch (const std::exception& e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    template <typename Self>
    bool saveToBinaryFile(this const Self& self, const std::string& fileName)
    {
        std::vector<uint8_t> buffer;
        self.toBinary(buffer);

        std::ofstream file(fileName, std::ios::binary);

        if (!file.is_open()) {
            const_cast<Self&>(self).lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        return true;
    }

    template <typename Self>
    bool loadFromBinaryFile(this Self& self, const std::string& fileName)
    {
        std::ifstream file(fileName, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            self.lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            self.lastErrorString = "Failed reading binary file stream";
            return false;
        }

        std::span<const uint8_t> span(buffer);
        return self.fromBinary(span);
    }

private:
    // =========================================================================
    // JSON Helpers
    // =========================================================================

    template <typename T>
    static nlohmann::json serializeJsonValue(const T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            return static_cast<std::underlying_type_t<V>>(val);
        } else if constexpr (std::same_as<V, std::byte>) {
            return static_cast<uint8_t>(val);
        } else if constexpr (ExtendedCharType<V>) {
            return static_cast<uint32_t>(val);
        } else if constexpr (SmartPointer<V>) {
            if (!val)
                return nullptr;

            return serializeJsonValue(*val);
        } else if constexpr (ReflectableContainer<V>) {
            nlohmann::json arr = nlohmann::json::array();

            for (const auto& item : val) {
                arr.push_back(serializeJsonValue(item));
            }

            return arr;
        } else if constexpr (BaseObjectType<V>) {
            return val.toJson();
        } else {
            return val;
        }
    }

    template <typename T>
    static void deserializeJsonValue(const nlohmann::json& j, T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            val = static_cast<V>(j.template get<std::underlying_type_t<V>>());
        } else if constexpr (std::same_as<V, std::byte>) {
            val = static_cast<std::byte>(j.template get<uint8_t>());
        } else if constexpr (ExtendedCharType<V>) {
            val = static_cast<V>(j.template get<uint32_t>());
        } else if constexpr (SmartPointer<V>) {
            if (j.is_null()) {
                val.reset();
            } else {
                using Elem = typename V::element_type;

                if constexpr (requires { Elem::createShared(); }) {
                    val = Elem::createShared();
                } else {
                    val = std::make_shared<Elem>();
                }

                deserializeJsonValue(j, *val);
            }
        } else if constexpr (ReflectableContainer<V>) {
            val.clear();

            for (const auto& elem : j) {
                typename V::value_type item{};
                deserializeJsonValue(elem, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromJson(j))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested JSON deserialization failed" : val.lastErrorString);
        } else {
            val = j.template get<V>();
        }
    }


    // =========================================================================
    // YAML CPP Helpers
    // =========================================================================

    template <typename T>
    static YAML::Node serializeYamlValue(const T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            YAML::Node n;
            n = static_cast<int64_t>(static_cast<std::underlying_type_t<V>>(val));
            return n;
        } else if constexpr (std::same_as<V, std::byte>) {
            YAML::Node n;
            n = static_cast<int>(val);
            return n;
        } else if constexpr (ExtendedCharType<V>) {
            YAML::Node n;
            n = static_cast<uint32_t>(val);
            return n;
        } else if constexpr (SmartPointer<V>) {
            if (!val)
                return YAML::Node(YAML::NodeType::Null);

            return serializeYamlValue(*val);
        } else if constexpr (ReflectableContainer<V>) {
            YAML::Node seq(YAML::NodeType::Sequence);

            for (const auto& item : val) {
                seq.push_back(serializeYamlValue(item));
            }

            return seq;
        } else if constexpr (BaseObjectType<V>) {
            return val.toYaml();
        } else {
            YAML::Node n;
            n = val;
            return n;
        }
    }

    template <typename T>
    static void deserializeYamlValue(const YAML::Node& node, T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            val = static_cast<V>(node.template as<std::underlying_type_t<V>>());
        } else if constexpr (std::same_as<V, std::byte>) {
            val = static_cast<std::byte>(node.template as<uint16_t>());
        } else if constexpr (ExtendedCharType<V>) {
            val = static_cast<V>(node.template as<uint32_t>());
        } else if constexpr (SmartPointer<V>) {
            if (node.IsNull()) {
                val.reset();
            } else {
                using Elem = typename V::element_type;

                if constexpr (requires { Elem::createShared(); }) {
                    val = Elem::createShared();
                } else {
                    val = std::make_shared<Elem>();
                }

                deserializeYamlValue(node, *val);
            }
        } else if constexpr (ReflectableContainer<V>) {
            val.clear();

            for (const auto& elem : node) {
                typename V::value_type item{};
                deserializeYamlValue(elem, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromYaml(node))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested YAML deserialization failed" : val.lastErrorString);
        } else {
            val = node.template as<V>();
        }
    }

    // =========================================================================
    // Binary Helpers
    // =========================================================================

    template <typename T>
    static void writeBinary(std::vector<uint8_t>& buf, const T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_trivially_copyable_v<V>) {
            const auto* ptr = reinterpret_cast<const uint8_t*>(&val);
            buf.insert(buf.end(), ptr, ptr + sizeof(V));
        } else if constexpr (std::same_as<V, std::string>) {
            uint64_t len = val.size();
            writeBinary(buf, len);
            buf.insert(buf.end(), val.data(), val.data() + val.size());
        } else if constexpr (SmartPointer<V>) {
            const bool present = static_cast<bool>(val);
            writeBinary(buf, present);

            if (present) {
                writeBinary(buf, *val);
            }
        } else if constexpr (MapContainer<V>) {
            uint64_t count = val.size();
            writeBinary(buf, count);

            for (const auto& [k, v] : val) {
                writeBinary(buf, k);
                writeBinary(buf, v);
            }
        } else if constexpr (ReflectableContainer<V>) {
            uint64_t count = val.size();
            writeBinary(buf, count);

            for (const auto& item : val) {
                writeBinary(buf, item);
            }
        } else if constexpr (BaseObjectType<V>) {
            val.toBinary(buf);
        }
    }

    template <typename T>
    static void readBinary(std::span<const uint8_t>& streamSpan, T& val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_trivially_copyable_v<V>) {
            if (streamSpan.size() < sizeof(V)) {
                throw std::runtime_error("Unexpected buffer EOF in binary stream");
            }

            std::memcpy(&val, streamSpan.data(), sizeof(V));
            streamSpan = streamSpan.subspan(sizeof(V));
        } else if constexpr (std::same_as<V, std::string>) {
            uint64_t len = 0;
            readBinary(streamSpan, len);

            if (streamSpan.size() < len) {
                throw std::runtime_error("Unexpected buffer EOF reading string");
            }

            val.assign(reinterpret_cast<const char*>(streamSpan.data()), len);
            streamSpan = streamSpan.subspan(len);
        } else if constexpr (SmartPointer<V>) {
            bool present = false;
            readBinary(streamSpan, present);

            if (present) {
                using Elem = typename V::element_type;

                if constexpr (requires { Elem::createShared(); }) {
                    val = Elem::createShared();
                } else {
                    val = std::make_shared<Elem>();
                }

                readBinary(streamSpan, *val);
            } else {
                val.reset();
            }
        } else if constexpr (MapContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);
            val.clear();

            for (uint64_t i = 0; i < count; ++i) {
                typename V::key_type key;
                typename V::mapped_type elem;

                readBinary(streamSpan, key);
                readBinary(streamSpan, elem);

                val.emplace(std::move(key), std::move(elem));
            }
        } else if constexpr (ReflectableContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);
            val.clear();

            for (uint64_t i = 0; i < count; ++i) {
                typename V::value_type item{};
                readBinary(streamSpan, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromBinary(streamSpan))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested binary deserialization failed" : val.lastErrorString);
        }
    }
};

} // namespace job::core

template <job::core::BaseObjectType T>
struct nlohmann::adl_serializer<T>
{
    static void to_json(json& j, const T& obj)
    {
        j = obj.toJson();
    }

    static void from_json(const json& j, T& obj)
    {
        obj.fromJson(j);
    }
};

namespace YAML {

template <job::core::BaseObjectType T>
struct convert<T>
{
    static Node encode(const T& rhs)
    {
        return rhs.toYaml();
    }

    static bool decode(const Node& node, T& rhs)
    {
        return rhs.fromYaml(node);
    }
};

} // namespace YAML