#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace job::json {

template <typename>
inline constexpr bool dependentFalseV = false;

// =============================================================================
// Basic type normalization
// =============================================================================

template <typename T>
using JsonType = std::remove_cvref_t<T>;

// =============================================================================
// Strings
// =============================================================================

template <typename T>
concept JsonOwnedString = std::same_as<JsonType<T>, std::string>;

template <typename T>
concept JsonStringView = std::same_as<JsonType<T>, std::string_view>;

template <typename T>
concept JsonString = JsonOwnedString<T> || JsonStringView<T>;

// =============================================================================
// Scalar types
// =============================================================================

template <typename T>
concept JsonBoolean = std::same_as<JsonType<T>, bool>;

template <typename T>
concept JsonEnum = std::is_enum_v<JsonType<T>>;

template <typename T>
concept JsonByte = std::same_as<JsonType<T>, std::byte>;

template <typename T>
concept JsonExtendedChar = std::same_as<JsonType<T>, wchar_t> ||
                           std::same_as<JsonType<T>, char8_t> ||
                           std::same_as<JsonType<T>, char16_t> ||
                           std::same_as<JsonType<T>, char32_t>;

template <typename T>
concept JsonSignedInteger =
    std::signed_integral<JsonType<T>> &&
    !JsonBoolean<T> &&
    !std::same_as<JsonType<T>, char> &&
    !JsonExtendedChar<T>;

template <typename T>
concept JsonUnsignedInteger =
    std::unsigned_integral<JsonType<T>> &&
    !JsonBoolean<T> &&
    !JsonExtendedChar<T>;

template <typename T>
concept JsonInteger = JsonSignedInteger<T> || JsonUnsignedInteger<T>;

template <typename T>
concept JsonFloatingPoint = std::floating_point<JsonType<T>>;

template <typename T>
concept JsonNumber = JsonInteger<T> || JsonFloatingPoint<T>;

template <typename T>
concept JsonScalar = JsonBoolean<T> || JsonNumber<T> || JsonEnum<T> ||
                     JsonByte<T> || JsonExtendedChar<T> || JsonString<T>;

// =============================================================================
// Optional
// =============================================================================

template <typename T>
struct JsonOptionalTraits
{
    static constexpr bool value = false;
};

template <typename T>
struct JsonOptionalTraits<std::optional<T>>
{
    static constexpr bool value = true;
    using ValueType = T;
};

template <typename T>
concept JsonOptional = JsonOptionalTraits<JsonType<T>>::value;

template <JsonOptional T>
using JsonOptionalValue = typename JsonOptionalTraits<JsonType<T>>::ValueType;

// =============================================================================
// Smart pointers
// =============================================================================

template <typename T>
struct JsonSharedPointerTraits
{
    static constexpr bool value = false;
};

template <typename T>
struct JsonSharedPointerTraits<std::shared_ptr<T>>
{
    static constexpr bool value = true;
    using ElementType = T;
};

template <typename T>
concept JsonSharedPointer = JsonSharedPointerTraits<JsonType<T>>::value;

template <typename T>
struct JsonUniquePointerTraits
{
    static constexpr bool value = false;
};

template <typename T, typename Deleter>
struct JsonUniquePointerTraits<std::unique_ptr<T, Deleter>>
{
    static constexpr bool value = true;
    using ElementType = T;
};

template <typename T>
concept JsonUniquePointer = JsonUniquePointerTraits<JsonType<T>>::value;

template <typename T>
struct JsonWeakPointerTraits
{
    static constexpr bool value = false;
};

template <typename T>
struct JsonWeakPointerTraits<std::weak_ptr<T>>
{
    static constexpr bool value = true;
    using ElementType = T;
};

template <typename T>
concept JsonWeakPointer = JsonWeakPointerTraits<JsonType<T>>::value;

template <typename T>
concept JsonOwningPointer = JsonSharedPointer<T> || JsonUniquePointer<T>;

template <typename T>
concept JsonUnsupportedPointer = std::is_pointer_v<JsonType<T>> || JsonWeakPointer<T>;

template <JsonSharedPointer T>
using JsonSharedPointerElement = typename JsonSharedPointerTraits<JsonType<T>>::ElementType;

template <JsonUniquePointer T>
using JsonUniquePointerElement = typename JsonUniquePointerTraits<JsonType<T>>::ElementType;

// =============================================================================
// Containers
// =============================================================================

template <typename T>
concept JsonContainerBase =
    !JsonString<T> &&
    requires(JsonType<T> &container, const JsonType<T> &constContainer) {
        typename JsonType<T>::value_type;

        { constContainer.begin() };
        { constContainer.end() };
    };

template <typename T>
concept JsonMap = JsonContainerBase<T> &&
                  requires {
                      typename JsonType<T>::key_type;
                      typename JsonType<T>::mapped_type;
                  } &&
                  requires(
                      JsonType<T> &container,
                      typename JsonType<T>::key_type key,
                      typename JsonType<T>::mapped_type value)
{
    container.clear();
    container.emplace(std::move(key), std::move(value));
};

template <typename T>
concept JsonFixedSequence =
    JsonContainerBase<T> &&
    !JsonMap<T> &&
    requires(JsonType<T> &container, std::size_t index) {
        { container.size() } -> std::convertible_to<std::size_t>;
        container[index];
    } &&
    !requires(JsonType<T> &container, typename JsonType<T>::value_type value) {
        container.clear();
        container.push_back(std::move(value));
    } &&
    !requires(JsonType<T> &container, typename JsonType<T>::value_type value) {
        container.clear();
        container.insert(std::move(value));
    };

template <typename T>
concept JsonPushBackSequence =
    JsonContainerBase<T> &&
    !JsonMap<T> &&
    requires(JsonType<T> &container, typename JsonType<T>::value_type value) {
        container.clear();
        container.push_back(std::move(value));
    };

template <typename T>
concept JsonInsertSequence =
    JsonContainerBase<T> &&
    !JsonMap<T> &&
    !JsonPushBackSequence<T> &&
    requires(JsonType<T> &container, typename JsonType<T>::value_type value) {
        container.clear();
        container.insert(std::move(value));
    };

template <typename T>
concept JsonSequence = JsonFixedSequence<T> || JsonPushBackSequence<T> || JsonInsertSequence<T>;

template <typename T>
concept JsonContainer = JsonMap<T> || JsonSequence<T>;

// =============================================================================
// Persistent values
// =============================================================================

template <typename T>
concept JsonValue =
    !JsonUnsupportedPointer<T> &&
    (
        JsonScalar<T> ||
        JsonOptional<T> ||
        JsonOwningPointer<T> ||
        JsonContainer<T> ||
        std::is_class_v<JsonType<T>>
        );

// =============================================================================
// Output sink
// =============================================================================

template <typename T>
concept JsonOutputSink = requires(T &sink, std::string_view text) {
    sink.append(text);
};




// =============================================================================
// Parse destinations
// =============================================================================

template <typename T>
concept JsonValueDestination =
    requires(T &destination, std::string_view borrowed, std::string owned, bool boolean) {
        { destination.null() } -> std::convertible_to<bool>;
        { destination.boolean(boolean) } -> std::convertible_to<bool>;
        { destination.number(borrowed) } -> std::convertible_to<bool>;
        { destination.string(borrowed) } -> std::convertible_to<bool>;
        { destination.stringOwned(std::move(owned)) } -> std::convertible_to<bool>;
    };

template <typename T>
concept JsonObjectDestination = requires(T &destination) {
    destination.object();
};

} // namespace job::json
