#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace job::yaml {

template <typename T>
using YamlType = std::remove_cvref_t<T>;

// Strings
template <typename T>
concept YamlOwnedString = std::same_as<YamlType<T>, std::string>;

template <typename T>
concept YamlStringView = std::same_as<YamlType<T>, std::string_view>;

template <typename T>
concept YamlString = YamlOwnedString<T> || YamlStringView<T>;

// Scalars
template <typename T>
concept YamlBool = std::same_as<YamlType<T>, bool>;

template <typename T>
concept YamlSignedInteger = std::signed_integral<YamlType<T>> && !YamlBool<T>;

template <typename T>
concept YamlUnsignedInteger = std::unsigned_integral<YamlType<T>> && !YamlBool<T>;

template <typename T>
concept YamlInteger = YamlSignedInteger<T> || YamlUnsignedInteger<T>;

template <typename T>
concept YamlFloatingPoint = std::floating_point<YamlType<T>>;

template <typename T>
concept YamlEnum = std::is_enum_v<YamlType<T>>;

template <typename T>
concept YamlScalar = YamlString<T> || YamlBool<T> || YamlInteger<T> || YamlFloatingPoint<T> || YamlEnum<T>;

// Optional
template <typename T>
struct IsYamlOptional : std::false_type {};

template <typename T>
struct IsYamlOptional<std::optional<T>> : std::true_type {};

template <typename T>
concept YamlOptional = IsYamlOptional<YamlType<T>>::value;

// Pointers
template <typename T>
struct IsYamlSharedPointer : std::false_type {};

template <typename T>
struct IsYamlSharedPointer<std::shared_ptr<T>> : std::true_type {};

template <typename T>
concept YamlSharedPointer = IsYamlSharedPointer<YamlType<T>>::value;

template <typename T>
struct IsYamlUniquePointer : std::false_type {};

template <typename T, typename Deleter>
struct IsYamlUniquePointer<std::unique_ptr<T, Deleter>> : std::true_type {};

template <typename T>
concept YamlUniquePointer = IsYamlUniquePointer<YamlType<T>>::value;

template <typename T>
concept YamlPointer = YamlSharedPointer<T> || YamlUniquePointer<T>;

// Mappings
template <typename T>
concept YamlMap =
    requires {
        typename YamlType<T>::key_type;
        typename YamlType<T>::mapped_type;
    } &&
    std::ranges::range<YamlType<T>>;

// Sequences
template <typename T>
concept YamlSequence = std::ranges::range<YamlType<T>> && !YamlString<T> && !YamlMap<T>;

// Broad YAML value categories
template <typename T>
concept YamlContainer = YamlSequence<T> || YamlMap<T>;

template <typename T>
concept YamlBasicValue = YamlScalar<T> || YamlOptional<T> || YamlPointer<T> || YamlContainer<T>;

// Type Shapes / Output
template <typename T>
concept YamlOutputSink = requires(T &sink, std::string_view text, char c) {
    sink.append(text);
    sink.push_back(c);
};

// Type Shapes / Parse destinations
template <typename T>
concept YamlParseDestination = requires(T &destination, std::string_view borrowed, std::string owned) {
    { destination.null() } -> std::convertible_to<bool>;

    { destination.scalar(borrowed) } -> std::convertible_to<bool>;
    { destination.scalarOwned(std::move(owned)) } -> std::convertible_to<bool>;

    { destination.key(borrowed) } -> std::convertible_to<bool>;
    { destination.keyOwned(std::move(owned)) } -> std::convertible_to<bool>;

    { destination.beginMapping() } -> std::convertible_to<bool>;
    { destination.endMapping() } -> std::convertible_to<bool>;

    { destination.beginSequence() } -> std::convertible_to<bool>;
    { destination.endSequence() } -> std::convertible_to<bool>;
};

template <typename T>
concept YamlNodeDestination = requires(T &node, T valueNode, std::string_view key, std::string_view value) {
    node.setNull();
    node.setScalar(value);
    node.setMapping();
    node.setSequence();

    node.setMember(key, std::move(valueNode));
    node.setMemberScalar(key, value);

    node.append(std::move(valueNode));
    node.appendScalar(value);
};

template <typename T>
concept YamlEventDestination = requires(T &sink, std::string_view key, std::string_view value) {
    sink.scalar(value);
    sink.member(key, value);
    sink.append(value);
};

} // namespace job::yaml