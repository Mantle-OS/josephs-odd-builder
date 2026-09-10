#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <job_json.h>
#include <job_yaml.h>
#include <job_binary.h>



#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "jobcore_export.h"
#include "job_obj_annotation.h"
#include "job_obj_concept.h"

namespace job::core {

class JOBCORE_EXPORT BaseObject
{
public:
    using Ptr  = std::shared_ptr<BaseObject>;
    using WPtr = std::weak_ptr<BaseObject>;
    using UPtr = std::unique_ptr<BaseObject>;

    BaseObject();
    virtual ~BaseObject();

    BaseObject(const BaseObject &);
    BaseObject &operator=(const BaseObject &);
    BaseObject(BaseObject &&) noexcept;
    BaseObject &operator=(BaseObject &&) noexcept;

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        std::string lastErrorString;


    // =========================================================================
    // JOB YAML Serialization
    // =========================================================================
    template <typename Self>
    bool toJobYaml(this const Self &self, std::string &yaml)
    {
        return job::yaml::JobYaml::toString(self, yaml);
    }

    template <typename Self>
    bool fromJobYaml(this Self &self, std::string_view yaml)
    {
        if (job::yaml::JobYaml::fromString(yaml, self))
            return true;

        self.lastErrorString = "Failed parsing JOB YAML";
        return false;
    }
    template <typename Self>
    bool saveToJobYamlFile(this const Self &self, const std::string &fileName)
    {
        std::string yaml;

        if (!self.toJobYaml(yaml)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB YAML";
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << yaml;

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromJobYamlFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        const std::string yaml{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };

        return self.fromJobYaml(yaml);
    }
    template <typename Self>
    void debugJobYaml(this const Self &self)
    {
        std::string yaml;

        if (self.toJobYaml(yaml))
            std::cout << "[JOB YAML]\n" << yaml << std::endl;
    }
    // =========================================================================
    // JOB JSON Serialization
    // =========================================================================

    template <typename Self>
    bool toJobJson(this const Self &self, std::string &json)
    {
        return job::json::JobJson::toJson(self, json);
    }

    template <typename Self>
    bool fromJobJson(this Self &self, std::string_view json)
    {
        job::json::JsonDiagnostic diagnostic;

        if (job::json::JobJson::fromJson(json, self, diagnostic))
            return true;

        self.lastErrorString = std::string{diagnostic.comment()};
        return false;
    }
    template <typename Self>
    bool saveToJobJsonFile(this const Self &self, const std::string &fileName)
    {
        std::string json;

        if (!self.toJobJson(json)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB JSON";
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << json;

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromJobJsonFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        const std::string json{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };

        return self.fromJobJson(json);
    }

    template <typename Self>
    void debugJobJson(this const Self &self)
    {
        std::string json;

        if (self.toJobJson(json))
            std::cout << "[JOB JSON]\n" << json << std::endl;
    }


    // =========================================================================
    // JOB Binary Serialization
    // =========================================================================

    template <typename Self>
    bool toBinary(this const Self &self, std::vector<std::uint8_t> &buffer)
    {
        return job::binary::JobBinary::toBytes(self, buffer);
    }

    template <typename Self>
    bool fromBinary(this Self &self, std::span<const std::uint8_t> streamSpan)
    {
        if (job::binary::JobBinary::fromBytes(streamSpan, self))
            return true;

        self.lastErrorString = "Failed parsing JOB binary data";
        return false;
    }
    template <typename Self>
    bool saveToBinaryFile(this const Self &self, const std::string &fileName)
    {
        std::vector<std::uint8_t> buffer;

        if (!self.toBinary(buffer)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB binary";
            return false;
        }

        std::ofstream file(fileName, std::ios::binary);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        file.write(reinterpret_cast<const char *>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing binary file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromBinaryFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            self.lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        const std::streamsize size = file.tellg();

        if (size < 0) {
            self.lastErrorString = "Failed determining binary file size: " + fileName;
            return false;
        }

        file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));

        if (size > 0 &&
            !file.read(reinterpret_cast<char *>(buffer.data()), size)) {
            self.lastErrorString = "Failed reading binary file stream";
            return false;
        }

        return self.fromBinary(std::span<const std::uint8_t>{buffer});
    }


    // ====================================
    // START 3rd party


    // =========================================================================
    // nlohmann JSON Serialization
    // =========================================================================

    template <typename Self>
    nlohmann::json toJson(this const Self &self)
    {
        nlohmann::json j = nlohmann::json::object();

        template for (constexpr auto member : reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member)) {
                constexpr auto name = std::meta::identifier_of(member);
                j[std::string(name)] = serializeJsonValue(self.[:member:]);
            }
        }

        return j;
    }

    template <typename Self>
    bool fromJson(this Self &self, const nlohmann::json &j)
    {
        try {
            if (!j.is_object()) {
                self.lastErrorString = "JSON value is not an object";
                return false;
            }

            template for (constexpr auto member : reflectedDataMembersV<Self>) {
                using MemberType = typename[:std::meta::type_of(member):];

                if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member)) {
                    constexpr auto name = std::meta::identifier_of(member);
                    const std::string key(name);

                    if (j.contains(key))
                        deserializeJsonValue(j[key], self.[:member:]);
                }
            }

            return true;
        } catch (const std::exception &e) {
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
    bool saveToJsonFile(this const Self &self, const std::string &fileName)
    {
        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << self.toJson().dump(4);

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromJsonFile(this Self &self, const std::string &fileName)
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
        } catch (const std::exception &e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    // =========================================================================
    // YAML CPP Serialization
    // =========================================================================

    template <typename Self>
    YAML::Node toYaml(this const Self &self)
    {
        YAML::Node node(YAML::NodeType::Map);

        template for (constexpr auto member : reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member)) {
                constexpr auto name = std::meta::identifier_of(member);
                node[std::string(name)] = serializeYamlValue(self.[:member:]);
            }
        }

        return node;
    }

    template <typename Self>
    bool fromYaml(this Self &self, const YAML::Node &node)
    {
        try {
            if (!node.IsMap()) {
                self.lastErrorString = "YAML node is not a map";
                return false;
            }

            template for (constexpr auto member : reflectedDataMembersV<Self>) {
                using MemberType = typename[:std::meta::type_of(member):];

                if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member)) {
                    constexpr auto name = std::meta::identifier_of(member);
                    const std::string key(name);

                    if (node[key])
                        deserializeYamlValue(node[key], self.[:member:]);
                }
            }

            return true;
        } catch (const std::exception &e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    template <typename Self>
    void debugYaml(this const Self &self)
    {
        YAML::Emitter emitter;
        emitter << self.toYaml();
        std::cout << "[YAML]\n" << emitter.c_str() << std::endl;
    }

    template <typename Self>
    bool saveToYamlFile(this const Self &self, const std::string &fileName)
    {
        YAML::Emitter out;
        out << self.toYaml();

        if (!out.good()) {
            const_cast<Self &>(self).lastErrorString = "YAML error: " + std::string(out.GetLastError());
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed opening file: " + fileName;
            return false;
        }

        file << out.c_str();

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromYamlFile(this Self &self, const std::string &fileName)
    {
        try {
            YAML::Node node = YAML::LoadFile(fileName);
            return self.fromYaml(node);
        } catch (const YAML::Exception &e) {
            self.lastErrorString = "Error parsing YAML file: " + std::string(e.what());
            return false;
        }
    }

    // // =========================================================================
    // // Binary Serialization
    // // =========================================================================

    // template <typename Self>
    // void toBinary(this const Self &self, std::vector<uint8_t> &buffer)
    // {
    //     template for (constexpr auto member : reflectedDataMembersV<Self>) {
    //         using MemberType = typename[:std::meta::type_of(member):];

    //         if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member))
    //             writeBinary(buffer, self.[:member:]);
    //     }
    // }

    // template <typename Self>
    // bool fromBinary(this Self &self, std::span<const uint8_t> &streamSpan)
    // {
    //     try {
    //         template for (constexpr auto member : reflectedDataMembersV<Self>) {
    //             using MemberType = typename[:std::meta::type_of(member):];

    //             if constexpr (!SignalType<MemberType> && !hasNoSerializeAnnotation(member))
    //                 readBinary(streamSpan, self.[:member:]);
    //         }

    //         return true;
    //     } catch (const std::exception &e) {
    //         self.lastErrorString = e.what();
    //         return false;
    //     }
    // }

    // template <typename Self>
    // bool saveToBinaryFile(this const Self &self, const std::string &fileName)
    // {
    //     std::vector<uint8_t> buffer;
    //     self.toBinary(buffer);

    //     std::ofstream file(fileName, std::ios::binary);

    //     if (!file.is_open()) {
    //         const_cast<Self &>(self).lastErrorString = "Failed opening binary file: " + fileName;
    //         return false;
    //     }

    //     file.write(reinterpret_cast<const char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

    //     if (!file.good()) {
    //         const_cast<Self &>(self).lastErrorString = "Failed writing binary file: " + fileName;
    //         return false;
    //     }

    //     return true;
    // }

    // template <typename Self>
    // bool loadFromBinaryFile(this Self &self, const std::string &fileName)
    // {
    //     std::ifstream file(fileName, std::ios::binary | std::ios::ate);

    //     if (!file.is_open()) {
    //         self.lastErrorString = "Failed opening binary file: " + fileName;
    //         return false;
    //     }

    //     const std::streamsize size = file.tellg();

    //     if (size < 0) {
    //         self.lastErrorString = "Failed determining binary file size: " + fileName;
    //         return false;
    //     }

    //     file.seekg(0, std::ios::beg);

    //     std::vector<uint8_t> buffer(static_cast<std::size_t>(size));

    //     if (size > 0 && !file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    //         self.lastErrorString = "Failed reading binary file stream";
    //         return false;
    //     }

    //     std::span<const uint8_t> span(buffer);
    //     return self.fromBinary(span);
    // }

private:
    template <OwningSmartPointer Pointer>
    static void constructPointer(Pointer &pointer)
    {
        using V = std::remove_cvref_t<Pointer>;
        using Elem = typename V::element_type;

        if constexpr (SharedPointer<V>) {
            if constexpr (requires { { Elem::createShared() } -> std::convertible_to<V>; })
                pointer = Elem::createShared();
            else
                pointer = std::make_shared<Elem>();
        } else if constexpr (UniquePointer<V>) {
            if constexpr (requires { { Elem::createUniq() } -> std::convertible_to<V>; }) {
                pointer = Elem::createUniq();
            } else if constexpr (std::constructible_from<V, Elem *>) {
                pointer = V(new Elem{});
            } else {
                static_assert(dependentFalseV<V>, "Unique pointer type cannot be reconstructed");
            }
        }
    }


    // =========================================================================
    // JOB YAML Helpers
    // =========================================================================
#if 0
    template <typename T>
    static job::yaml::YamlNode serializeJobYamlValue(const T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            return serializeJobYamlScalar(static_cast<std::underlying_type_t<V>>(val));
        } else if constexpr (std::same_as<V, std::byte>) {
            return serializeJobYamlScalar(static_cast<uint8_t>(val));
        } else if constexpr (ExtendedCharType<V>) {
            return serializeJobYamlScalar(static_cast<uint32_t>(val));
        } else if constexpr (OptionalType<V>) {
            if (!val) {
                job::yaml::YamlNode node;
                job::yaml::JobYaml::null(node);
                return node;
            }

            return serializeJobYamlValue(*val);
        } else if constexpr (OwningSmartPointer<V>) {
            if (!val) {
                job::yaml::YamlNode node;
                job::yaml::JobYaml::null(node);
                return node;
            }

            return serializeJobYamlValue(*val);
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be serialized");
        } else if constexpr (MapContainer<V>) {
            job::yaml::YamlNode sequence;
            job::yaml::JobYaml::sequence(sequence);

            for (const auto &[key, value] : val) {
                job::yaml::YamlNode entry;
                job::yaml::JobYaml::sequence(entry);

                job::yaml::JobYaml::append(entry, serializeJobYamlValue(key));
                job::yaml::JobYaml::append(entry, serializeJobYamlValue(value));

                job::yaml::JobYaml::append(sequence, std::move(entry));
            }

            return sequence;
        } else if constexpr (PersistentContainer<V>) {
            job::yaml::YamlNode sequence;
            job::yaml::JobYaml::sequence(sequence);

            for (const auto &item : val)
                job::yaml::JobYaml::append(sequence, serializeJobYamlValue(item));

            return sequence;
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JOB YAML serialization");
        } else if constexpr (BaseObjectType<V>) {
            return val.toJobYaml();
        } else {
            return serializeJobYamlScalar(val);
        }
    }

    template <typename T>
    static void deserializeJobYamlValue(const job::yaml::YamlNode &node, T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            std::underlying_type_t<V> value{};

            if (!deserializeJobYamlScalar(node, value))
                throw std::runtime_error("Invalid JOB YAML enum value");

            val = static_cast<V>(value);
        } else if constexpr (std::same_as<V, std::byte>) {
            uint8_t value{};

            if (!deserializeJobYamlScalar(node, value))
                throw std::runtime_error("Invalid JOB YAML byte value");

            val = static_cast<std::byte>(value);
        } else if constexpr (ExtendedCharType<V>) {
            uint32_t value{};

            if (!deserializeJobYamlScalar(node, value))
                throw std::runtime_error("Invalid JOB YAML character value");

            val = static_cast<V>(value);
        } else if constexpr (OptionalType<V>) {
            if (node.isNull()) {
                val.reset();
            } else {
                val.emplace();
                deserializeJobYamlValue(node, *val);
            }
        } else if constexpr (OwningSmartPointer<V>) {
            if (node.isNull()) {
                val.reset();
            } else {
                constructPointer(val);
                deserializeJobYamlValue(node, *val);
            }
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be deserialized");
        } else if constexpr (MapContainer<V>) {
            if (!node.isSequence())
                throw std::runtime_error("JOB YAML map value is not a sequence");

            val.clear();

            for (const auto &entry : node.sequence()) {
                if (!entry.isSequence() || entry.sequence().size() != 2)
                    throw std::runtime_error("JOB YAML map entry must contain exactly two values");

                typename V::key_type key{};
                typename V::mapped_type value{};

                deserializeJobYamlValue(entry.sequence()[0], key);
                deserializeJobYamlValue(entry.sequence()[1], value);

                val.emplace(std::move(key), std::move(value));
            }
        } else if constexpr (FixedSequenceContainer<V>) {
            if (!node.isSequence())
                throw std::runtime_error("JOB YAML array value is not a sequence");

            if (node.sequence().size() != val.size())
                throw std::runtime_error("JOB YAML fixed array size mismatch");

            for (std::size_t i = 0; i < val.size(); ++i)
                deserializeJobYamlValue(node.sequence()[i], val[i]);
        } else if constexpr (PushBackSequenceContainer<V>) {
            if (!node.isSequence())
                throw std::runtime_error("JOB YAML container value is not a sequence");

            val.clear();

            for (const auto &elem : node.sequence()) {
                typename V::value_type item{};
                deserializeJobYamlValue(elem, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (InsertSequenceContainer<V>) {
            if (!node.isSequence())
                throw std::runtime_error("JOB YAML container value is not a sequence");

            val.clear();

            for (const auto &elem : node.sequence()) {
                typename V::value_type item{};
                deserializeJobYamlValue(elem, item);
                val.insert(std::move(item));
            }
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JOB YAML deserialization");
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromJobYaml(node))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested JOB YAML deserialization failed" : val.lastErrorString);
        } else {
            if (!deserializeJobYamlScalar(node, val))
                throw std::runtime_error("Invalid JOB YAML scalar value");
        }
    }

    template <typename T>
    static job::yaml::YamlNode serializeJobYamlScalar(const T &val)
    {
        using V = std::remove_cvref_t<T>;

        job::yaml::YamlNode node;

        if constexpr (std::same_as<V, std::string> || std::same_as<V, std::string_view>) {
            if (!job::yaml::JobYaml::node(node, val))
                throw std::runtime_error("Failed serializing JOB YAML string");

            return node;
        } else {
            std::string scalar;

            if (!job::yaml::JobYaml::emit(val, scalar))
                throw std::runtime_error("Failed serializing JOB YAML scalar");

            if (!job::yaml::JobYaml::node(node, scalar))
                throw std::runtime_error("Failed constructing JOB YAML scalar node");

            return node;
        }
    }

    template <typename T>
    static bool deserializeJobYamlScalar(const job::yaml::YamlNode &node, T &val)
    {
        if (!node.isScalar())
            return false;

        return job::yaml::JobYaml::assign(val, node.scalar());
    }


    // =========================================================================
    // JOB JSON Helpers
    // =========================================================================

    template <typename T>
    static std::string serializeJobJsonValue(const T &val)
    {
        using V = std::remove_cvref_t<T>;

        std::string json;

        if constexpr (std::is_enum_v<V>) {
            if (!job::json::JsonEmitter::emit(static_cast<std::underlying_type_t<V>>(val), json))
                throw std::runtime_error("Failed serializing JOB JSON enum value");
        } else if constexpr (std::same_as<V, std::byte>) {
            if (!job::json::JsonEmitter::emit(static_cast<uint8_t>(val), json))
                throw std::runtime_error("Failed serializing JOB JSON byte value");
        } else if constexpr (ExtendedCharType<V>) {
            if (!job::json::JsonEmitter::emit(static_cast<uint32_t>(val), json))
                throw std::runtime_error("Failed serializing JOB JSON character value");
        } else if constexpr (OptionalType<V>) {
            if (!val)
                return "null";

            return serializeJobJsonValue(*val);
        } else if constexpr (OwningSmartPointer<V>) {
            if (!val)
                return "null";

            return serializeJobJsonValue(*val);
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be serialized");
        } else if constexpr (MapContainer<V>) {
            if (!job::json::JsonEmitter::emit(val, json))
                throw std::runtime_error("Failed serializing JOB JSON map value");
        } else if constexpr (PersistentContainer<V>) {
            if (!job::json::JsonEmitter::emit(val, json))
                throw std::runtime_error("Failed serializing JOB JSON container value");
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JOB JSON serialization");
        } else if constexpr (BaseObjectType<V>) {
            if (!job::json::JsonEmitter::emit(val, json))
                throw std::runtime_error("Failed serializing nested JOB JSON object");
        } else {
            if (!job::json::JsonEmitter::emit(val, json))
                throw std::runtime_error("Failed serializing JOB JSON value");
        }

        return json;
    }

    template <typename T>
    static void deserializeJobJsonValue(std::string_view json, T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            std::underlying_type_t<V> value{};
            job::json::JsonParser parser{json};

            if (!parser.parse(value))
                throw std::runtime_error("Invalid JOB JSON enum value");

            val = static_cast<V>(value);
        } else if constexpr (std::same_as<V, std::byte>) {
            uint8_t value{};
            job::json::JsonParser parser{json};

            if (!parser.parse(value))
                throw std::runtime_error("Invalid JOB JSON byte value");

            val = static_cast<std::byte>(value);
        } else if constexpr (ExtendedCharType<V>) {
            uint32_t value{};
            job::json::JsonParser parser{json};

            if (!parser.parse(value))
                throw std::runtime_error("Invalid JOB JSON character value");

            val = static_cast<V>(value);
        } else if constexpr (OptionalType<V>) {
            if (json == "null") {
                val.reset();
            } else {
                val.emplace();
                deserializeJobJsonValue(json, *val);
            }
        } else if constexpr (OwningSmartPointer<V>) {
            if (json == "null") {
                val.reset();
            } else {
                constructPointer(val);
                deserializeJobJsonValue(json, *val);
            }
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be deserialized");
        } else if constexpr (MapContainer<V>) {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Invalid JOB JSON map value");
        } else if constexpr (FixedSequenceContainer<V>) {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Invalid JOB JSON fixed array value");
        } else if constexpr (PushBackSequenceContainer<V>) {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Invalid JOB JSON container value");
        } else if constexpr (InsertSequenceContainer<V>) {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Invalid JOB JSON container value");
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JOB JSON deserialization");
        } else if constexpr (BaseObjectType<V>) {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Nested JOB JSON deserialization failed");
        } else {
            job::json::JsonParser parser{json};

            if (!parser.parse(val))
                throw std::runtime_error("Invalid JOB JSON value");
        }
    }
    // =========================================================================
    // Binary Helpers
    // =========================================================================
    template <typename T>
    static void writeBinary(std::vector<uint8_t> &buf, const T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (OptionalType<V>) {
            const bool present = val.has_value();
            writeBinary(buf, present);

            if (present)
                writeBinary(buf, *val);
        } else if constexpr (OwningSmartPointer<V>) {
            const bool present = static_cast<bool>(val);
            writeBinary(buf, present);

            if (present)
                writeBinary(buf, *val);
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be serialized");
        } else if constexpr (std::same_as<V, std::string>) {
            const uint64_t len = val.size();
            writeBinary(buf, len);
            buf.insert(buf.end(), val.data(), val.data() + val.size());
        } else if constexpr (MapContainer<V>) {
            const uint64_t count = val.size();
            writeBinary(buf, count);

            for (const auto &[key, value] : val) {
                writeBinary(buf, key);
                writeBinary(buf, value);
            }
        } else if constexpr (PersistentContainer<V>) {
            const uint64_t count = val.size();
            writeBinary(buf, count);

            for (const auto &item : val)
                writeBinary(buf, item);
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in binary serialization");
        } else if constexpr (BaseObjectType<V>) {
            val.toBinary(buf);
        } else if constexpr (std::is_trivially_copyable_v<V>) {
            const auto *ptr = reinterpret_cast<const uint8_t *>(&val);
            buf.insert(buf.end(), ptr, ptr + sizeof(V));
        } else {
            static_assert(dependentFalseV<V>, "Unsupported type in binary serialization");
        }
    }

    template <typename T>
    static void readBinary(std::span<const uint8_t> &streamSpan, T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (OptionalType<V>) {
            bool present = false;
            readBinary(streamSpan, present);

            if (present) {
                val.emplace();
                readBinary(streamSpan, *val);
            } else {
                val.reset();
            }
        } else if constexpr (OwningSmartPointer<V>) {
            bool present = false;
            readBinary(streamSpan, present);

            if (present) {
                constructPointer(val);
                readBinary(streamSpan, *val);
            } else {
                val.reset();
            }
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be deserialized");
        } else if constexpr (std::same_as<V, std::string>) {
            uint64_t len = 0;
            readBinary(streamSpan, len);

            if (streamSpan.size() < len)
                throw std::runtime_error("Unexpected buffer EOF reading string");

            val.assign(reinterpret_cast<const char *>(streamSpan.data()), static_cast<std::size_t>(len));
            streamSpan = streamSpan.subspan(static_cast<std::size_t>(len));
        } else if constexpr (MapContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);
            val.clear();

            for (uint64_t i = 0; i < count; ++i) {
                typename V::key_type key{};
                typename V::mapped_type value{};
                readBinary(streamSpan, key);
                readBinary(streamSpan, value);
                val.emplace(std::move(key), std::move(value));
            }
        } else if constexpr (FixedSequenceContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);

            if (count != val.size())
                throw std::runtime_error("Binary fixed array size mismatch");

            for (auto &item : val)
                readBinary(streamSpan, item);
        } else if constexpr (PushBackSequenceContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);
            val.clear();

            for (uint64_t i = 0; i < count; ++i) {
                typename V::value_type item{};
                readBinary(streamSpan, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (InsertSequenceContainer<V>) {
            uint64_t count = 0;
            readBinary(streamSpan, count);
            val.clear();

            for (uint64_t i = 0; i < count; ++i) {
                typename V::value_type item{};
                readBinary(streamSpan, item);
                val.insert(std::move(item));
            }
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in binary deserialization");
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromBinary(streamSpan))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested binary deserialization failed" : val.lastErrorString);
        } else if constexpr (std::is_trivially_copyable_v<V>) {
            if (streamSpan.size() < sizeof(V))
                throw std::runtime_error("Unexpected buffer EOF in binary stream");

            std::memcpy(&val, streamSpan.data(), sizeof(V));
            streamSpan = streamSpan.subspan(sizeof(V));
        } else {
            static_assert(dependentFalseV<V>, "Unsupported type in binary deserialization");
        }
    }
#endif


    // 3rd party


    // =========================================================================
    // JSON Helpers
    // =========================================================================

    template <typename T>
    static nlohmann::json serializeJsonValue(const T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            return static_cast<std::underlying_type_t<V>>(val);
        } else if constexpr (std::same_as<V, std::byte>) {
            return static_cast<uint8_t>(val);
        } else if constexpr (ExtendedCharType<V>) {
            return static_cast<uint32_t>(val);
        } else if constexpr (OptionalType<V>) {
            if (!val)
                return nullptr;

            return serializeJsonValue(*val);
        } else if constexpr (OwningSmartPointer<V>) {
            if (!val)
                return nullptr;

            return serializeJsonValue(*val);
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be serialized");
        } else if constexpr (MapContainer<V>) {
            nlohmann::json arr = nlohmann::json::array();

            for (const auto &[key, value] : val) {
                nlohmann::json entry = nlohmann::json::array();
                entry.push_back(serializeJsonValue(key));
                entry.push_back(serializeJsonValue(value));
                arr.push_back(std::move(entry));
            }

            return arr;
        } else if constexpr (PersistentContainer<V>) {
            nlohmann::json arr = nlohmann::json::array();

            for (const auto &item : val)
                arr.push_back(serializeJsonValue(item));

            return arr;
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JSON serialization");
        } else if constexpr (BaseObjectType<V>) {
            return val.toJson();
        } else {
            return val;
        }
    }

    template <typename T>
    static void deserializeJsonValue(const nlohmann::json &j, T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            val = static_cast<V>(j.template get<std::underlying_type_t<V>>());
        } else if constexpr (std::same_as<V, std::byte>) {
            val = static_cast<std::byte>(j.template get<uint8_t>());
        } else if constexpr (ExtendedCharType<V>) {
            val = static_cast<V>(j.template get<uint32_t>());
        } else if constexpr (OptionalType<V>) {
            if (j.is_null()) {
                val.reset();
            } else {
                val.emplace();
                deserializeJsonValue(j, *val);
            }
        } else if constexpr (OwningSmartPointer<V>) {
            if (j.is_null()) {
                val.reset();
            } else {
                constructPointer(val);
                deserializeJsonValue(j, *val);
            }
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be deserialized");
        } else if constexpr (MapContainer<V>) {
            if (!j.is_array())
                throw std::runtime_error("JSON map value is not an array");

            val.clear();

            for (const auto &entry : j) {
                if (!entry.is_array() || entry.size() != 2)
                    throw std::runtime_error("JSON map entry must contain exactly two values");

                typename V::key_type key{};
                typename V::mapped_type value{};
                deserializeJsonValue(entry[0], key);
                deserializeJsonValue(entry[1], value);
                val.emplace(std::move(key), std::move(value));
            }
        } else if constexpr (FixedSequenceContainer<V>) {
            if (!j.is_array())
                throw std::runtime_error("JSON array value is not an array");

            if (j.size() != val.size())
                throw std::runtime_error("JSON fixed array size mismatch");

            for (std::size_t i = 0; i < val.size(); ++i)
                deserializeJsonValue(j[i], val[i]);
        } else if constexpr (PushBackSequenceContainer<V>) {
            if (!j.is_array())
                throw std::runtime_error("JSON container value is not an array");

            val.clear();

            for (const auto &elem : j) {
                typename V::value_type item{};
                deserializeJsonValue(elem, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (InsertSequenceContainer<V>) {
            if (!j.is_array())
                throw std::runtime_error("JSON container value is not an array");

            val.clear();

            for (const auto &elem : j) {
                typename V::value_type item{};
                deserializeJsonValue(elem, item);
                val.insert(std::move(item));
            }
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in JSON deserialization");
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
    static YAML::Node serializeYamlValue(const T &val)
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
        } else if constexpr (OptionalType<V>) {
            if (!val)
                return YAML::Node(YAML::NodeType::Null);

            return serializeYamlValue(*val);
        } else if constexpr (OwningSmartPointer<V>) {
            if (!val)
                return YAML::Node(YAML::NodeType::Null);

            return serializeYamlValue(*val);
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be serialized");
        } else if constexpr (MapContainer<V>) {
            YAML::Node seq(YAML::NodeType::Sequence);

            for (const auto &[key, value] : val) {
                YAML::Node entry(YAML::NodeType::Sequence);
                entry.push_back(serializeYamlValue(key));
                entry.push_back(serializeYamlValue(value));
                seq.push_back(entry);
            }

            return seq;
        } else if constexpr (PersistentContainer<V>) {
            YAML::Node seq(YAML::NodeType::Sequence);

            for (const auto &item : val)
                seq.push_back(serializeYamlValue(item));

            return seq;
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in YAML serialization");
        } else if constexpr (BaseObjectType<V>) {
            return val.toYaml();
        } else {
            YAML::Node n;
            n = val;
            return n;
        }
    }

    template <typename T>
    static void deserializeYamlValue(const YAML::Node &node, T &val)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (std::is_enum_v<V>) {
            val = static_cast<V>(node.template as<std::underlying_type_t<V>>());
        } else if constexpr (std::same_as<V, std::byte>) {
            val = static_cast<std::byte>(node.template as<uint16_t>());
        } else if constexpr (ExtendedCharType<V>) {
            val = static_cast<V>(node.template as<uint32_t>());
        } else if constexpr (OptionalType<V>) {
            if (!node || node.IsNull()) {
                val.reset();
            } else {
                val.emplace();
                deserializeYamlValue(node, *val);
            }
        } else if constexpr (OwningSmartPointer<V>) {
            if (!node || node.IsNull()) {
                val.reset();
            } else {
                constructPointer(val);
                deserializeYamlValue(node, *val);
            }
        } else if constexpr (UnsupportedPersistentPointer<V>) {
            static_assert(dependentFalseV<V>, "Raw and weak pointers cannot be deserialized");
        } else if constexpr (MapContainer<V>) {
            if (!node.IsSequence())
                throw std::runtime_error("YAML map value is not a sequence");

            val.clear();

            for (const auto &entry : node) {
                if (!entry.IsSequence() || entry.size() != 2)
                    throw std::runtime_error("YAML map entry must contain exactly two values");

                typename V::key_type key{};
                typename V::mapped_type value{};
                deserializeYamlValue(entry[0], key);
                deserializeYamlValue(entry[1], value);
                val.emplace(std::move(key), std::move(value));
            }
        } else if constexpr (FixedSequenceContainer<V>) {
            if (!node.IsSequence())
                throw std::runtime_error("YAML array value is not a sequence");

            if (node.size() != val.size())
                throw std::runtime_error("YAML fixed array size mismatch");

            for (std::size_t i = 0; i < val.size(); ++i)
                deserializeYamlValue(node[i], val[i]);
        } else if constexpr (PushBackSequenceContainer<V>) {
            if (!node.IsSequence())
                throw std::runtime_error("YAML container value is not a sequence");

            val.clear();

            for (const auto &elem : node) {
                typename V::value_type item{};
                deserializeYamlValue(elem, item);
                val.push_back(std::move(item));
            }
        } else if constexpr (InsertSequenceContainer<V>) {
            if (!node.IsSequence())
                throw std::runtime_error("YAML container value is not a sequence");

            val.clear();

            for (const auto &elem : node) {
                typename V::value_type item{};
                deserializeYamlValue(elem, item);
                val.insert(std::move(item));
            }
        } else if constexpr (ReflectableContainer<V>) {
            static_assert(dependentFalseV<V>, "Unsupported container type in YAML deserialization");
        } else if constexpr (BaseObjectType<V>) {
            if (!val.fromYaml(node))
                throw std::runtime_error(val.lastErrorString.empty() ? "Nested YAML deserialization failed" : val.lastErrorString);
        } else {
            val = node.template as<V>();
        }
    }
};

} // namespace job::core

template <job::core::BaseObjectType T>
struct nlohmann::adl_serializer<T>
{
    static void to_json(json &j, const T &obj)
    {
        j = obj.toJson();
    }

    static void from_json(const json &j, T &obj)
    {
        obj.fromJson(j);
    }
};

namespace YAML {

template <job::core::BaseObjectType T>
struct convert<T>
{
    static Node encode(const T &rhs)
    {
        return rhs.toYaml();
    }

    static bool decode(const Node &node, T &rhs)
    {
        return rhs.fromYaml(node);
    }
};

} // namespace YAML