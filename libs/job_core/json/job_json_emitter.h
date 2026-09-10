#pragma once

#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <meta>
#include <string_view>
#include <type_traits>

#include "job_json_concepts.h"  // keep for now [[JOSEPH]] merge into job_obj_concept.h maybe after shape and alpha->beta time
#include "job_obj_annotation.h"
#include "job_obj_concept.h"

#include "jobcore_export.h"

namespace job::json {

class JOBCORE_EXPORT JsonEmitter
{
public:
    JsonEmitter() = delete;
    ~JsonEmitter() = delete;

    JsonEmitter(const JsonEmitter &) = delete;
    JsonEmitter &operator=(const JsonEmitter &) = delete;
    JsonEmitter(JsonEmitter &&) = delete;
    JsonEmitter &operator=(JsonEmitter &&) = delete;

    template <JsonValue T, JsonOutputSink Sink>
    [[nodiscard]] static bool emit(const T &value, Sink &sink)
    {
        return emitValue(value, sink);
    }

private:
    template <typename T, JsonOutputSink Sink>
    [[nodiscard]] static bool emitValue(const T &value, Sink &sink)
    {
        using ValueType = JsonType<T>;

        if constexpr (JsonOptional<ValueType>) {
            if (!value) {
                append(sink, "null");
                return true;
            }

            return emitValue(*value, sink);
        } else if constexpr (JsonSharedPointer<ValueType> || JsonUniquePointer<ValueType>) {
            if (!value) {
                append(sink, "null");
                return true;
            }

            return emitValue(*value, sink);
        } else if constexpr (JsonBoolean<ValueType>) {
            append(sink, value ? "true" : "false");
            return true;
        } else if constexpr (JsonOwnedString<ValueType> || JsonStringView<ValueType>) {
            return emitString(std::string_view{value.data(), value.size()}, sink);
        } else if constexpr (JsonEnum<ValueType>) {
            using UnderlyingType = std::underlying_type_t<ValueType>;
            return emitNumber(static_cast<UnderlyingType>(value), sink);
        } else if constexpr (JsonByte<ValueType>) {
            return emitNumber(static_cast<std::uint8_t>(value), sink);
        } else if constexpr (JsonExtendedChar<ValueType>) {
            if constexpr (std::numeric_limits<ValueType>::is_signed) {
                return emitNumber(static_cast<std::int64_t>(value), sink);
            } else {
                return emitNumber(static_cast<std::uint64_t>(value), sink);
            }
        } else if constexpr (JsonNumber<ValueType>) {
            return emitNumber(value, sink);
        } else if constexpr (JsonMap<ValueType>) {
            return emitMap(value, sink);
        } else if constexpr (JsonContainer<ValueType>) {
            return emitSequence(value, sink);
        } else if constexpr (std::is_class_v<ValueType>) {
            return emitObject(value, sink);
        } else {
            static_assert(
                std::is_same_v<ValueType, void>,
                "JsonEmitter encountered unsupported value type");

            return false;
        }
    }

    template <typename T, JsonOutputSink Sink>
    [[nodiscard]] static bool emitNumber(const T &value, Sink &sink)
    {
        char buffer[64];

        const auto result = std::to_chars(
            buffer,
            buffer + sizeof(buffer),
            value);

        if (result.ec != std::errc{})
            return false;

        const std::string_view text{
            buffer,
            static_cast<std::size_t>(result.ptr - buffer)
        };

        if constexpr (std::floating_point<T>) {
            std::size_t offset = 0;

            if (!text.empty() && (text.front() == '-' || text.front() == '+'))
                offset = 1;

            if (offset < text.size()) {
                const char first = text[offset];

                if (first == 'i' || first == 'I' ||
                    first == 'n' || first == 'N')
                    return false;
            }
        }

        append(sink, text);
        return true;
    }

    template <JsonOutputSink Sink>
    [[nodiscard]] static bool emitString(std::string_view value, Sink &sink)
    {
        append(sink, "\"");

        for (const unsigned char character : value) {
            switch (character) {
            case '"':
                append(sink, "\\\"");
                break;

            case '\\':
                append(sink, "\\\\");
                break;

            case '\b':
                append(sink, "\\b");
                break;

            case '\f':
                append(sink, "\\f");
                break;

            case '\n':
                append(sink, "\\n");
                break;

            case '\r':
                append(sink, "\\r");
                break;

            case '\t':
                append(sink, "\\t");
                break;

            default:
                if (character < 0x20) {
                    emitControlCharacter(character, sink);
                } else {
                    const char byte = static_cast<char>(character);
                    append(sink, std::string_view{&byte, 1});
                }

                break;
            }
        }

        append(sink, "\"");
        return true;
    }

    template <typename T, JsonOutputSink Sink>
    [[nodiscard]] static bool emitObject(const T &object, Sink &sink)
    {
        using ObjectType = JsonType<T>;

        append(sink, "{");

        bool first = true;
        bool success = true;

        template for (constexpr auto member : job::core::reflectedDataMembersV<ObjectType>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (job::core::SignalType<MemberType> || job::core::hasNoSerializeAnnotation(member))
                continue;

            if (!success)
                continue;

            if (!first)
                append(sink, ",");

            constexpr std::string_view name = std::meta::identifier_of(member);

            success = emitString(name, sink);

            if (!success)
                continue;

            append(sink, ":");

            success = emitValue(object.[:member:], sink);
            first = false;
        }

        if (!success)
            return false;

        append(sink, "}");
        return true;
    }

    template <JsonContainer T, JsonOutputSink Sink>
    [[nodiscard]] static bool emitSequence(const T &container, Sink &sink)
    {
        append(sink, "[");

        bool first = true;

        for (const auto &value : container) {
            if (!first)
                append(sink, ",");

            if (!emitValue(value, sink))
                return false;

            first = false;
        }

        append(sink, "]");
        return true;
    }

    template <JsonMap T, JsonOutputSink Sink>
    [[nodiscard]] static bool emitMap(const T &container, Sink &sink)
    {
        append(sink, "[");

        bool first = true;

        for (const auto &[key, value] : container) {
            if (!first)
                append(sink, ",");

            append(sink, "[");

            if (!emitValue(key, sink))
                return false;

            append(sink, ",");

            if (!emitValue(value, sink))
                return false;

            append(sink, "]");

            first = false;
        }

        append(sink, "]");

        return true;
    }

    template <JsonOutputSink Sink>
    static void emitControlCharacter(unsigned char character, Sink &sink)
    {
        static constexpr char hex[] = "0123456789ABCDEF";

        const char escaped[] = {
            '\\',
            'u',
            '0',
            '0',
            hex[(character >> 4) & 0x0F],
            hex[character & 0x0F]
        };

        append(sink, std::string_view{escaped, sizeof(escaped)});
    }

    template <JsonOutputSink Sink>
    static void append(Sink &sink, std::string_view value)
    {
        sink.append(value);
    }
};

} // namespace job::json