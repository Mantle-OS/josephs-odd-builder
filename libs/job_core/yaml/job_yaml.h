#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "job_yaml_concepts.h"
#include "job_yaml_contracts.h"
#include "job_yaml_cursor.h"
#include "job_yaml_emitter.h"
#include "job_yaml_escape_kernel.h"
#include "job_yaml_event_sink.h"
#include "job_yaml_grammar.h"
#include "job_yaml_indent_stack.h"
#include "job_yaml_key_dispatch.h"
#include "job_yaml_lex.h"
#include "job_yaml_node.h"
#include "job_yaml_node_sink.h"
#include "job_yaml_object_reader.h"
#include "job_yaml_object_sink.h"
#include "job_yaml_scalar_kernel.h"
#include "job_yaml_sink.h"
#include "job_yaml_validate_sink.h"
#include "job_yaml_parser_state.h"
#include "job_yaml_parser_destination.h"
#include "job_yaml_object_parser_destination.h"
#include "job_yaml_parser.h"

#include "job_obj_concept.h"

namespace job::yaml {

class JobYaml
{
public:
    template <typename T, YamlOutputSink Sink>
    static constexpr bool emit(const T &value, Sink &sink)
    {
        using ValueType = YamlType<T>;

        if constexpr (std::same_as<ValueType, YamlNode> ||
                      YamlScalar<ValueType> ||
                      YamlOptional<ValueType> ||
                      YamlPointer<ValueType>) {
            return YamlEmitter::emitValue(value, sink);
        } else if constexpr (YamlContainer<ValueType> ||
                             job::core::BaseObjectType<ValueType>) {
            YamlNode node;

            if (!serializeValue(node, value))
                return false;

            return YamlEmitter::emitNode(node, sink);
        } else {
            return YamlEmitter::emitObject(value, sink);
        }
    }

    template <typename T>
    [[nodiscard]] static bool toString(const T &value, std::string &yaml)
    {
        yaml.clear();
        return emit(value, yaml);
    }

    template <typename T>
    [[nodiscard]] static bool fromString(std::string_view yaml, T &destination)
    {
        using DestinationType = YamlType<T>;

        if constexpr (std::same_as<DestinationType, YamlNode>) {
            YamlNodeParserDestination parserDestination{destination};
            return YamlParser::parse(yaml, parserDestination);
        } else if constexpr (job::core::BaseObjectType<DestinationType> ||
                             job::core::ReflectableContainer<DestinationType> ||
                             job::core::OptionalType<DestinationType> ||
                             job::core::OwningSmartPointer<DestinationType>) {
            YamlNode node;
            YamlNodeParserDestination parserDestination{node};

            if (!YamlParser::parse(yaml, parserDestination))
                return false;

            return deserializeValue(node, destination);
        } else {
            YamlObjectParserDestination<DestinationType> parserDestination{destination};
            return YamlParser::parse(yaml, parserDestination);
        }
    }

    template <typename T>
    static constexpr bool assign(T &destination, std::string_view value) noexcept
    {
        return YamlSink::scalar(destination, value);
    }

    template <typename T>
    static constexpr bool assign(T &destination, std::string_view key, std::string_view value) noexcept
    {
        return YamlObjectSink::member(destination, key, value);
    }

    template <typename T>
    static constexpr bool validate(std::string_view value) noexcept
    {
        return YamlValidateSink::scalar<T>(value);
    }

    template <typename T>
    static constexpr bool validate(std::string_view key, std::string_view value) noexcept
    {
        return YamlValidateSink::member<T>(key, value);
    }

    static constexpr bool null(YamlNode &destination) noexcept(noexcept(YamlNodeSink::null(destination)))
    {
        return YamlNodeSink::null(destination);
    }

    static constexpr bool node(YamlNode &destination, std::string_view value) noexcept(noexcept(YamlNodeSink::scalar(destination, value)))
    {
        return YamlNodeSink::scalar(destination, value);
    }

    static constexpr bool mapping(YamlNode &destination) noexcept(noexcept(YamlNodeSink::mapping(destination)))
    {
        return YamlNodeSink::mapping(destination);
    }

    static constexpr bool sequence(YamlNode &destination) noexcept(noexcept(YamlNodeSink::sequence(destination)))
    {
        return YamlNodeSink::sequence(destination);
    }

    static constexpr bool node(YamlNode &destination, std::string_view key, std::string_view value) noexcept(noexcept(YamlNodeSink::member(destination, key, value)))
    {
        return YamlNodeSink::member(destination, key, value);
    }

    static constexpr bool node(YamlNode &destination, std::string_view key, YamlNode value) noexcept(noexcept(YamlNodeSink::member(destination, key, std::move(value))))
    {
        return YamlNodeSink::member(destination, key, std::move(value));
    }

    static constexpr bool append(YamlNode &destination, std::string_view value) noexcept(noexcept(YamlNodeSink::append(destination, value)))
    {
        return YamlNodeSink::append(destination, value);
    }

    static constexpr bool append(YamlNode &destination, YamlNode value) noexcept(noexcept(YamlNodeSink::append(destination, std::move(value))))
    {
        return YamlNodeSink::append(destination, std::move(value));
    }

private:
    template <typename T>
    static bool serializeValue(YamlNode &destination, const T &value)
    {
        using ValueType = YamlType<T>;

        if constexpr (std::same_as<ValueType, YamlNode>) {
            destination = value;
            return true;
        } else if constexpr (std::is_enum_v<ValueType>) {
            return serializeScalar(destination, static_cast<std::underlying_type_t<ValueType>>(value));
        } else if constexpr (std::same_as<ValueType, std::byte>) {
            return serializeScalar(destination, static_cast<std::uint8_t>(value));
        } else if constexpr (job::core::ExtendedCharType<ValueType>) {
            return serializeScalar(destination, static_cast<std::uint32_t>(value));
        } else if constexpr (job::core::OptionalType<ValueType>) {
            if (!value)
                return null(destination);

            return serializeValue(destination, *value);
        } else if constexpr (job::core::OwningSmartPointer<ValueType>) {
            if (!value)
                return null(destination);

            return serializeValue(destination, *value);
        } else if constexpr (job::core::UnsupportedPersistentPointer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Raw and weak pointers cannot be serialized");
        } else if constexpr (job::core::MapContainer<ValueType>) {
            if (!sequence(destination))
                return false;

            for (const auto &[key, mappedValue] : value) {
                YamlNode entry;

                if (!sequence(entry))
                    return false;

                YamlNode keyNode;

                if (!serializeValue(keyNode, key))
                    return false;

                if (!append(entry, std::move(keyNode)))
                    return false;

                YamlNode valueNode;

                if (!serializeValue(valueNode, mappedValue))
                    return false;

                if (!append(entry, std::move(valueNode)))
                    return false;

                if (!append(destination, std::move(entry)))
                    return false;
            }

            return true;
        } else if constexpr (job::core::PersistentContainer<ValueType>) {
            if (!sequence(destination))
                return false;

            for (const auto &item : value) {
                YamlNode itemNode;

                if (!serializeValue(itemNode, item))
                    return false;

                if (!append(destination, std::move(itemNode)))
                    return false;
            }

            return true;
        } else if constexpr (job::core::ReflectableContainer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported container type in JOB YAML serialization");
        } else if constexpr (job::core::BaseObjectType<ValueType>) {
            if (!mapping(destination))
                return false;

            template for (constexpr auto member : job::core::reflectedDataMembersV<ValueType>) {
                if constexpr (job::core::isSerializableMember<member>()) {
                    constexpr std::string_view name = std::meta::identifier_of(member);

                    YamlNode memberNode;

                    if (!serializeValue(memberNode, value.[:member:]))
                        return false;

                    if (!node(destination, name, std::move(memberNode)))
                        return false;
                }
            }

            return true;
        } else {
            return serializeScalar(destination, value);
        }
    }

    template <typename T>
    static bool deserializeValue(const YamlNode &node, T &value)
    {
        using ValueType = YamlType<T>;

        if constexpr (std::is_enum_v<ValueType>) {
            std::underlying_type_t<ValueType> scalar{};

            if (!deserializeScalar(node, scalar))
                return false;

            value = static_cast<ValueType>(scalar);
            return true;
        } else if constexpr (std::same_as<ValueType, std::byte>) {
            std::uint8_t scalar{};

            if (!deserializeScalar(node, scalar))
                return false;

            value = static_cast<std::byte>(scalar);
            return true;
        } else if constexpr (job::core::ExtendedCharType<ValueType>) {
            std::uint32_t scalar{};

            if (!deserializeScalar(node, scalar))
                return false;

            value = static_cast<ValueType>(scalar);
            return true;
        } else if constexpr (job::core::OptionalType<ValueType>) {
            if (node.isNull()) {
                value.reset();
                return true;
            }

            value.emplace();
            return deserializeValue(node, *value);
        } else if constexpr (job::core::OwningSmartPointer<ValueType>) {
            if (node.isNull()) {
                value.reset();
                return true;
            }

            constructPointer(value);
            return deserializeValue(node, *value);
        } else if constexpr (job::core::UnsupportedPersistentPointer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Raw and weak pointers cannot be deserialized");
        } else if constexpr (job::core::MapContainer<ValueType>) {
            if (!node.isSequence())
                return false;

            value.clear();

            for (const auto &entry : node.sequence()) {
                if (!entry.isSequence() || entry.sequence().size() != 2)
                    return false;

                typename ValueType::key_type key{};
                typename ValueType::mapped_type mappedValue{};

                if (!deserializeValue(entry.sequence()[0], key))
                    return false;

                if (!deserializeValue(entry.sequence()[1], mappedValue))
                    return false;

                value.emplace(std::move(key), std::move(mappedValue));
            }

            return true;
        } else if constexpr (job::core::FixedSequenceContainer<ValueType>) {
            if (!node.isSequence())
                return false;

            if (node.sequence().size() != value.size())
                return false;

            for (std::size_t i = 0; i < value.size(); ++i) {
                if (!deserializeValue(node.sequence()[i], value[i]))
                    return false;
            }

            return true;
        } else if constexpr (job::core::PushBackSequenceContainer<ValueType>) {
            if (!node.isSequence())
                return false;

            value.clear();

            for (const auto &element : node.sequence()) {
                typename ValueType::value_type item{};

                if (!deserializeValue(element, item))
                    return false;

                value.push_back(std::move(item));
            }

            return true;
        } else if constexpr (job::core::InsertSequenceContainer<ValueType>) {
            if (!node.isSequence())
                return false;

            value.clear();

            for (const auto &element : node.sequence()) {
                typename ValueType::value_type item{};

                if (!deserializeValue(element, item))
                    return false;

                value.insert(std::move(item));
            }

            return true;
        } else if constexpr (job::core::ReflectableContainer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported container type in JOB YAML deserialization");
        } else if constexpr (job::core::BaseObjectType<ValueType>) {
            if (!node.isMapping())
                return false;

            bool success = true;

            template for (constexpr auto member : job::core::reflectedDataMembersV<ValueType>) {
                if constexpr (job::core::isSerializableMember<member>()) {
                    constexpr std::string_view name = std::meta::identifier_of(member);

                    if (const auto *memberNode = node.member(name)) {
                        if (!deserializeValue(*memberNode, value.[:member:]))
                            success = false;
                    }
                }
            }

            return success;
        } else {
            return deserializeScalar(node, value);
        }
    }

    template <typename T>
    static bool serializeScalar(YamlNode &destination, const T &value)
    {
        using ValueType = YamlType<T>;

        if constexpr (YamlString<ValueType>) {
            return node(destination, value);
        } else {
            std::string scalar;

            if (!YamlEmitter::emitScalar(value, scalar))
                return false;

            return node(destination, scalar);
        }
    }

    template <typename T>
    static bool deserializeScalar(const YamlNode &node, T &value)
    {
        if (!node.isScalar())
            return false;

        return assign(value, node.scalar());
    }

    template <job::core::OwningSmartPointer Pointer>
    static void constructPointer(Pointer &pointer)
    {
        using PointerType = YamlType<Pointer>;
        using ElementType = typename PointerType::element_type;

        if constexpr (job::core::SharedPointer<PointerType>) {
            if constexpr (requires { ElementType::createShared(); })
                pointer = ElementType::createShared();
            else
                pointer = std::make_shared<ElementType>();
        } else if constexpr (job::core::UniquePointer<PointerType>) {
            if constexpr (requires { ElementType::createUniq(); })
                pointer = ElementType::createUniq();
            else
                pointer = std::make_unique<ElementType>();
        }
    }
};

} // namespace job::yaml
