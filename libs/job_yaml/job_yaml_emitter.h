#pragma once

#include <bit>
#include <charconv>
#include <cstdint>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>

#include "job_yaml_concepts.h"
#include "job_yaml_node.h"

namespace job::yaml {

class YamlEmitter
{
public:
    template <typename T, YamlOutputSink Sink>
    static constexpr bool emitObject(const T &object, Sink &sink)
    {
        bool success = true;

        template for (constexpr auto member : std::define_static_array(
                          std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()))) {
            using MemberType = std::remove_cvref_t<decltype(object.[:member:])>;

            static_assert(isSupportedValue<MemberType>(),
                          "YamlEmitter encountered unsupported member type");

            if (!success)
                continue;

            constexpr std::string_view name = std::meta::identifier_of(member);

            sink.append(name);
            sink.append(": ");

            success = emitValue(object.[:member:], sink);

            if (success)
                sink.push_back('\n');
        }

        return success;
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitNode(const YamlNode &node, Sink &sink)
    {
        return emitNode(node, sink, 0);
    }

    template <typename T, YamlOutputSink Sink>
    static constexpr bool emitValue(const T &value, Sink &sink)
    {
        using ValueType = YamlType<T>;

        if constexpr (std::same_as<ValueType, YamlNode>) {
            return emitNode(value, sink);
        } else if constexpr (YamlOptional<ValueType>) {
            if (!value)
                return emitNull(sink);

            return emitValue(*value, sink);
        } else if constexpr (YamlPointer<ValueType>) {
            if (!value)
                return emitNull(sink);

            return emitValue(*value, sink);
        } else if constexpr (YamlScalar<ValueType>) {
            return emitScalar(value, sink);
        } else {
            return false;
        }
    }

    template <typename T, YamlOutputSink Sink>
    static constexpr bool emitScalar(const T &value, Sink &sink)
    {
        using ValueType = YamlType<T>;
        if constexpr (YamlBool<ValueType>) {
            sink.append(value ? std::string_view{"true"} : std::string_view{"false"});
            return true;
        } else if constexpr (std::is_enum_v<ValueType>) {
            using Underlying = std::underlying_type_t<ValueType>;
            return emitInteger(static_cast<Underlying>(value), sink);
        } else if constexpr (YamlInteger<ValueType>) {
            return emitInteger(value, sink);
        } else if constexpr (YamlFloatingPoint<ValueType>) {
            return emitFloat(value, sink);
        } else if constexpr (YamlString<ValueType>) {
            return emitString(value, sink);
        } else {
            return false;
        }
    }

private:
    template <typename T>
    [[nodiscard]] static consteval bool isSupportedValue() noexcept
    {
        using ValueType = YamlType<T>;

        if constexpr (std::same_as<ValueType, YamlNode>) {
            return true;
        } else if constexpr (YamlOptional<ValueType>) {
            return isSupportedValue<typename ValueType::value_type>();
        } else if constexpr (YamlPointer<ValueType>) {
            return isSupportedValue<typename ValueType::element_type>();
        } else {
            return YamlScalar<ValueType>;
        }
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitNull(Sink &sink)
    {
        sink.append("null");
        return true;
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitNode(const YamlNode &node, Sink &sink, std::size_t indent)
    {
        switch (node.type()) {
        case YamlNode::Type::Null:
            return emitNull(sink);

        case YamlNode::Type::Scalar:
            return emitString(node.scalar(), sink);

        case YamlNode::Type::Mapping:
            return emitMapping(node.mapping(), sink, indent);

        case YamlNode::Type::Sequence:
            return emitSequence(node.sequence(), sink, indent);
        }

        return false;
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitMapping(const YamlNode::Mapping &mapping, Sink &sink, std::size_t indent)
    {
        if (mapping.empty()) {
            sink.append("{}");
            return true;
        }

        for (const auto &entry : mapping) {
            appendIndent(sink, indent);

            if (!emitString(entry.key, sink))
                return false;

            sink.push_back(':');

            if (entry.value.isScalar() || entry.value.isNull()) {
                sink.push_back(' ');

                if (!emitNode(entry.value, sink, indent + 2))
                    return false;

                sink.push_back('\n');
                continue;
            }

            sink.push_back('\n');

            if (!emitNode(entry.value, sink, indent + 2))
                return false;
        }

        return true;
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitSequence(const YamlNode::Sequence &sequence, Sink &sink, std::size_t indent)
    {
        if (sequence.empty()) {
            sink.append("[]");
            return true;
        }

        for (const auto &value : sequence) {
            appendIndent(sink, indent);
            sink.push_back('-');

            if (value.isScalar() || value.isNull()) {
                sink.push_back(' ');

                if (!emitNode(value, sink, indent + 2))
                    return false;

                sink.push_back('\n');
                continue;
            }

            sink.push_back('\n');

            if (!emitNode(value, sink, indent + 2))
                return false;
        }

        return true;
    }

    template <YamlOutputSink Sink>
    static constexpr void appendIndent(Sink &sink, std::size_t indent)
    {
        for (std::size_t i = 0; i < indent; ++i)
            sink.push_back(' ');
    }

    template <YamlInteger T, YamlOutputSink Sink>
    static constexpr bool emitInteger(T value, Sink &sink)
    {
        char buffer[64];

        const auto [ptr, ec] =
            std::to_chars(buffer, buffer + sizeof(buffer), value);

        if (ec != std::errc{})
            return false;

        sink.append(std::string_view{
            buffer,
            static_cast<std::size_t>(ptr - buffer)
        });

        return true;
    }

    template <YamlFloatingPoint T, YamlOutputSink Sink>
    static bool emitFloat(T value, Sink &sink)
    {
        if constexpr (std::same_as<YamlType<T>, float>) {
            const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);

            constexpr std::uint32_t ExponentMask = 0x7F800000U;
            constexpr std::uint32_t MantissaMask = 0x007FFFFFU;
            constexpr std::uint32_t SignMask = 0x80000000U;

            if ((bits & ExponentMask) == ExponentMask) {
                if ((bits & MantissaMask) != 0) {
                    sink.append(".nan");
                    return true;
                }

                sink.append((bits & SignMask) != 0 ? "-.inf" : ".inf");
                return true;
            }
        } else if constexpr (std::same_as<YamlType<T>, double>) {
            const std::uint64_t bits = std::bit_cast<std::uint64_t>(value);

            constexpr std::uint64_t ExponentMask = 0x7FF0000000000000ULL;
            constexpr std::uint64_t MantissaMask = 0x000FFFFFFFFFFFFFULL;
            constexpr std::uint64_t SignMask = 0x8000000000000000ULL;

            if ((bits & ExponentMask) == ExponentMask) {
                if ((bits & MantissaMask) != 0) {
                    sink.append(".nan");
                    return true;
                }

                sink.append((bits & SignMask) != 0 ? "-.inf" : ".inf");
                return true;
            }
        }

        char buffer[128];

        const auto [ptr, ec] =
            std::to_chars(
                buffer,
                buffer + sizeof(buffer),
                value,
                std::chars_format::general);

        if (ec != std::errc{})
            return false;

        sink.append(std::string_view{
            buffer,
            static_cast<std::size_t>(ptr - buffer)
        });

        return true;
    }

    template <YamlOutputSink Sink>
    static constexpr bool emitString(std::string_view value, Sink &sink)
    {
        static constexpr char Hex[] = "0123456789ABCDEF";

        sink.push_back('"');

        for (const unsigned char c : value) {
            switch (c) {
            case 0x00:
                sink.append("\\0");
                break;
            case 0x07:
                sink.append("\\a");
                break;
            case 0x08:
                sink.append("\\b");
                break;
            case 0x09:
                sink.append("\\t");
                break;
            case 0x0A:
                sink.append("\\n");
                break;
            case 0x0B:
                sink.append("\\v");
                break;
            case 0x0C:
                sink.append("\\f");
                break;
            case 0x0D:
                sink.append("\\r");
                break;
            case 0x1B:
                sink.append("\\e");
                break;
            case '"':
                sink.append("\\\"");
                break;
            case '\\':
                sink.append("\\\\");
                break;
            default:
                if (c < 0x20 || c == 0x7F) {
                    sink.append("\\x");
                    sink.push_back(Hex[(c >> 4) & 0x0F]);
                    sink.push_back(Hex[c & 0x0F]);
                } else {
                    sink.push_back(static_cast<char>(c));
                }

                break;
            }
        }

        sink.push_back('"');

        return true;
    }
};

} // namespace job::yaml