#pragma once

#include <string_view>
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
        } else {
            return YamlEmitter::emitObject(value, sink);
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
};

} // namespace job::yaml