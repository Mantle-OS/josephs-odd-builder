#pragma once

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "job_json_concepts.h"
#include "job_json_diagnostic.h"
#include "job_json_lex.h"
#include "job_json_parser_destination.h"
#include "job_json_string_kernel.h"

namespace job::json {

class JsonParser
{
public:
    explicit constexpr JsonParser(std::string_view source, std::size_t maxDepth = 128) noexcept :
        m_source(source),
        m_lexer(source),
        m_maxDepth(maxDepth)
    {
    }

    constexpr ~JsonParser() = default;

    JsonParser(const JsonParser &) = delete;
    JsonParser &operator=(const JsonParser &) = delete;
    constexpr JsonParser(JsonParser &&) noexcept = default;
    JsonParser &operator=(JsonParser &&) = delete;

    [[nodiscard]] constexpr std::string_view source() const noexcept
    {
        return m_source;
    }

    [[nodiscard]] constexpr std::size_t maxDepth() const noexcept
    {
        return m_maxDepth;
    }

    [[nodiscard]] constexpr const JsonDiagnostic &diagnostic() const noexcept
    {
        return m_diagnostic;
    }

    template <JsonValue T>
    [[nodiscard]] bool parse(T &value)
    {
        m_diagnostic = {};

        if (!parseValue(value, 0))
            return false;

        const JsonLex lex = m_lexer.next();

        if (lex.type() == JsonLexType::Invalid)
            return fail(JsonDiagnosticCode::InvalidToken, lex.range(), "Invalid token after JSON value");

        if (lex.type() != JsonLexType::End)
            return fail(JsonDiagnosticCode::UnexpectedToken, lex.range(), "Unexpected token after JSON value");

        return true;
    }

private:
    template <typename T>
    [[nodiscard]] bool parseValue(T &value, std::size_t depth)
    {
        const JsonLex lex = m_lexer.next();
        return parseValue(value, lex, depth);
    }

    template <typename T>
    [[nodiscard]] bool parseValue(T &value, const JsonLex &lex, std::size_t depth)
    {
        JsonParserDestination<T> destination{value};

        switch (lex.type()) {
        case JsonLexType::Null:
            if (!destination.null())
                return fail(JsonDiagnosticCode::InvalidNull, lex.range(), "JSON null is not valid for destination");

            return true;

        case JsonLexType::True:
            if (!destination.boolean(true))
                return fail(JsonDiagnosticCode::TypeMismatch, lex.range(), "JSON boolean does not match destination");

            return true;

        case JsonLexType::False:
            if (!destination.boolean(false))
                return fail(JsonDiagnosticCode::TypeMismatch, lex.range(), "JSON boolean does not match destination");

            return true;

        case JsonLexType::Number:
            if (!destination.number(lex.view(m_source)))
                return fail(JsonDiagnosticCode::TypeMismatch, lex.range(), "JSON number does not match destination");

            return true;

        case JsonLexType::String:
            return parseString(destination, lex);

        case JsonLexType::BeginObject:
            if (depth >= m_maxDepth)
                return fail(JsonDiagnosticCode::MaxDepthExceeded, lex.range(), "Maximum JSON nesting depth exceeded");

            return destination.withValue([&](auto &inner) {
                return parseObject(inner.value(), depth + 1);
            });

        case JsonLexType::BeginArray:
            if (depth >= m_maxDepth)
                return fail(JsonDiagnosticCode::MaxDepthExceeded, lex.range(), "Maximum JSON nesting depth exceeded");

            return destination.withValue([&](auto &inner) {
                return parseArray(inner.value(), depth + 1);
            });

        case JsonLexType::End:
            return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON value");

        case JsonLexType::Invalid:
            return fail(JsonDiagnosticCode::InvalidToken, lex.range(), "Invalid JSON token");

        default:
            return fail(JsonDiagnosticCode::ExpectedValue, lex.range(), "Expected JSON value");
        }
    }

    template <typename T>
    [[nodiscard]] bool parseString(
        JsonParserDestination<T> &destination,
        const JsonLex &lex)
    {
        if (!lex.hasEscapes()) {
            if (!destination.string(JsonStringKernel::borrowed(lex, m_source)))
                return fail(JsonDiagnosticCode::TypeMismatch, lex.range(), "JSON string does not match destination");

            return true;
        }

        std::string decoded;

        if (!JsonStringKernel::decode(lex, m_source, decoded))
            return fail(JsonDiagnosticCode::InvalidUnicode, lex.range(), "Invalid JSON Unicode escape");

        if (!destination.stringOwned(std::move(decoded)))
            return fail(JsonDiagnosticCode::TypeMismatch, lex.range(), "Decoded JSON string does not match destination");

        return true;
    }

    template <typename T>
    [[nodiscard]] bool parseObject(T &object, std::size_t depth)
    {
        using ObjectType = JsonType<T>;

        if constexpr (
            JsonScalar<ObjectType> ||
            JsonOptional<ObjectType> ||
            JsonOwningPointer<ObjectType> ||
            JsonContainer<ObjectType>) {
            return failCurrent(JsonDiagnosticCode::TypeMismatch, "JSON object does not match destination");
        } else if constexpr (std::is_class_v<ObjectType>) {
            JsonObjectParserDestination<ObjectType> destination{object};

            JsonLex lex = m_lexer.next();

            if (lex.type() == JsonLexType::EndObject)
                return true;

            while (true) {
                if (lex.type() == JsonLexType::End)
                    return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected object key");

                if (lex.type() == JsonLexType::Invalid)
                    return fail(JsonDiagnosticCode::InvalidToken, lex.range(), "Invalid token in JSON object");

                if (lex.type() != JsonLexType::String)
                    return fail(JsonDiagnosticCode::ExpectedObjectKey, lex.range(), "Expected JSON object key");

                const JsonSourceRange keyRange = lex.range();

                std::string ownedKey;
                std::string_view key;

                if (lex.hasEscapes()) {
                    if (!JsonStringKernel::decode(lex, m_source, ownedKey))
                        return fail(JsonDiagnosticCode::InvalidUnicode, lex.range(), "Invalid Unicode escape in object key");

                    key = ownedKey;
                } else {
                    key = JsonStringKernel::borrowed(lex, m_source);
                }

                lex = m_lexer.next();
                if (lex.type() != JsonLexType::NameSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedNameSeparator,
                        lex.range(),
                        "Expected ':' after object key");

                const auto result = destination.dispatch(key, [&](auto &member) {
                    return parseValue(member, depth);
                });

                if (result == JsonKeyDispatchResult::NotFound)
                    return fail(JsonDiagnosticCode::UnknownMember, keyRange, "Unknown JSON object member");

                if (result == JsonKeyDispatchResult::Rejected)
                    return false;

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::EndObject)
                    return true;

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ',' or '}' before end of JSON object");

                if (lex.type() != JsonLexType::ValueSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedValueSeparator,
                        lex.range(),
                        "Expected ',' or '}' in JSON object");

                lex = m_lexer.next();
            }
        } else {
            return failCurrent(JsonDiagnosticCode::TypeMismatch, "JSON object does not match destination");
        }
    }

    template <typename T>
    [[nodiscard]] bool parseArray(T &container, std::size_t depth)
    {
        using ContainerType = JsonType<T>;

        if constexpr (JsonMap<ContainerType>) {
            return parseMap(container, depth);
        } else if constexpr (JsonFixedSequence<ContainerType>) {
            return parseFixedSequence(container, depth);
        } else if constexpr (JsonPushBackSequence<ContainerType>) {
            return parsePushBackSequence(container, depth);
        } else if constexpr (JsonInsertSequence<ContainerType>) {
            return parseInsertSequence(container, depth);
        } else {
            return failCurrent(JsonDiagnosticCode::TypeMismatch, "JSON array does not match destination");
        }
    }

    template <JsonFixedSequence T>
    [[nodiscard]] bool parseFixedSequence(T &container, std::size_t depth)
    {
        JsonLex lex = m_lexer.next();

        if (lex.type() == JsonLexType::EndArray) {
            if (container.size() != 0)
                return fail(JsonDiagnosticCode::ContainerError, lex.range(), "JSON array has too few elements");

            return true;
        }

        if (lex.type() == JsonLexType::End)
            return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");

        std::size_t index = 0;

        while (true) {
            if (index >= container.size())
                return fail(JsonDiagnosticCode::ContainerError, lex.range(), "JSON array has too many elements");

            if (!parseValue(container[index], lex, depth))
                return false;

            ++index;
            lex = m_lexer.next();

            if (lex.type() == JsonLexType::EndArray) {
                if (index != container.size())
                    return fail(JsonDiagnosticCode::ContainerError, lex.range(), "JSON array has too few elements");

                return true;
            }

            if (lex.type() == JsonLexType::End)
                return fail(
                    JsonDiagnosticCode::UnexpectedEnd,
                    lex.range(),
                    "Expected ',' or ']' before end of JSON array");

            if (lex.type() != JsonLexType::ValueSeparator)
                return fail(
                    JsonDiagnosticCode::ExpectedValueSeparator,
                    lex.range(),
                    "Expected ',' or ']' in JSON array");

            lex = m_lexer.next();

            if (lex.type() == JsonLexType::End)
                return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");

            if (lex.type() == JsonLexType::EndArray)
                return fail(JsonDiagnosticCode::ExpectedValue, lex.range(), "Expected JSON array element");
        }
    }

    template <JsonPushBackSequence T>
    [[nodiscard]] bool parsePushBackSequence(T &container, std::size_t depth)
    {
        using ElementType = typename JsonType<T>::value_type;

        if constexpr (!std::default_initializable<ElementType>) {
            return failCurrent(
                JsonDiagnosticCode::ContainerError,
                "JSON array element is not default constructible");
        } else {
            container.clear();

            JsonLex lex = m_lexer.next();

            if (lex.type() == JsonLexType::EndArray)
                return true;

            if (lex.type() == JsonLexType::End)
                return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");

            while (true) {
                ElementType element{};

                if (!parseValue(element, lex, depth))
                    return false;

                container.push_back(std::move(element));

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::EndArray)
                    return true;

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ',' or ']' before end of JSON array");

                if (lex.type() != JsonLexType::ValueSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedValueSeparator,
                        lex.range(),
                        "Expected ',' or ']' in JSON array");

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::End)
                    return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");
            }
        }
    }

    template <JsonInsertSequence T>
    [[nodiscard]] bool parseInsertSequence(T &container, std::size_t depth)
    {
        using ElementType = typename JsonType<T>::value_type;

        if constexpr (!std::default_initializable<ElementType>) {
            return failCurrent(
                JsonDiagnosticCode::ContainerError,
                "JSON array element is not default constructible");
        } else {
            container.clear();

            JsonLex lex = m_lexer.next();

            if (lex.type() == JsonLexType::EndArray)
                return true;

            if (lex.type() == JsonLexType::End)
                return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");

            while (true) {
                ElementType element{};

                if (!parseValue(element, lex, depth))
                    return false;

                container.insert(std::move(element));

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::EndArray)
                    return true;

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ',' or ']' before end of JSON array");

                if (lex.type() != JsonLexType::ValueSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedValueSeparator,
                        lex.range(),
                        "Expected ',' or ']' in JSON array");

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::End)
                    return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON array element");
            }
        }
    }

    template <JsonMap T>
    [[nodiscard]] bool parseMap(T &container, std::size_t depth)
    {
        using KeyType = typename JsonType<T>::key_type;
        using MappedType = typename JsonType<T>::mapped_type;

        if constexpr (
            !std::default_initializable<KeyType> ||
            !std::default_initializable<MappedType>) {
            return failCurrent(
                JsonDiagnosticCode::ContainerError,
                "JSON map key or value is not default constructible");
        } else {
            container.clear();

            JsonLex lex = m_lexer.next();

            if (lex.type() == JsonLexType::EndArray)
                return true;

            if (lex.type() == JsonLexType::End)
                return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON map entry");

            while (true) {
                if (lex.type() != JsonLexType::BeginArray)
                    return fail(JsonDiagnosticCode::ContainerError, lex.range(), "Expected JSON map entry array");

                KeyType key{};
                MappedType mapped{};

                if (!parseValue(key, depth))
                    return false;

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ',' between JSON map key and value");

                if (lex.type() != JsonLexType::ValueSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedValueSeparator,
                        lex.range(),
                        "Expected ',' between JSON map key and value");

                if (!parseValue(mapped, depth))
                    return false;

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ']' after JSON map entry");

                if (lex.type() != JsonLexType::EndArray)
                    return fail(
                        JsonDiagnosticCode::ExpectedArrayEnd,
                        lex.range(),
                        "Expected ']' after JSON map entry");

                container.emplace(std::move(key), std::move(mapped));

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::EndArray)
                    return true;

                if (lex.type() == JsonLexType::End)
                    return fail(
                        JsonDiagnosticCode::UnexpectedEnd,
                        lex.range(),
                        "Expected ',' or ']' after JSON map entry");

                if (lex.type() != JsonLexType::ValueSeparator)
                    return fail(
                        JsonDiagnosticCode::ExpectedValueSeparator,
                        lex.range(),
                        "Expected ',' or ']' after JSON map entry");

                lex = m_lexer.next();

                if (lex.type() == JsonLexType::End)
                    return fail(JsonDiagnosticCode::UnexpectedEnd, lex.range(), "Expected JSON map entry");
            }
        }
    }

    [[nodiscard]] bool fail(
        JsonDiagnosticCode code,
        JsonSourceRange range,
        std::string_view comment)
    {
        m_diagnostic = JsonDiagnostic{code, range, comment};
        return false;
    }

    [[nodiscard]] bool failCurrent(
        JsonDiagnosticCode code,
        std::string_view comment)
    {
        return fail(code, JsonSourceRange{m_source.size(), m_source.size()}, comment);
    }

    std::string_view m_source;
    JsonLexer        m_lexer;
    std::size_t      m_maxDepth{128};
    JsonDiagnostic   m_diagnostic{};
};

} // namespace job::json

