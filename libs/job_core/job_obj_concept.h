#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <meta>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace job::core {

class BaseObject;
class LightObject;
class Object;

template <typename... Args>
class Signal;

// =============================================================================
// General Type Helpers
// =============================================================================

template <typename T>
inline constexpr bool dependentFalseV = false;

// =============================================================================
// Light Object Type Requirements
// =============================================================================

template <typename T>
concept LightObjectType =
    std::is_class_v<T> &&
    std::derived_from<T, LightObject> &&
    std::default_initializable<T> &&
    std::destructible<T> &&
    !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> &&
    !std::move_constructible<T> &&
    !std::is_move_assignable_v<T>;

// =============================================================================
// Object Type Requirements
// =============================================================================

// Pure single-inheritance and pinned memory semantics.
template <typename T>
concept ObjectType =
    std::is_class_v<T> &&
    std::derived_from<T, Object> &&
    std::default_initializable<T> &&
    std::destructible<T> &&
    !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> &&
    !std::move_constructible<T> &&
    !std::is_move_assignable_v<T>;

// Pure serialization model.
template <typename T>
concept BaseObjectType = std::derived_from<T, BaseObject>;

// =============================================================================
// Optional Traits & Concept
// =============================================================================

template <typename T>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename T>
inline constexpr bool isOptionalTypeV = IsOptional<std::remove_cvref_t<T>>::value;

template <typename T>
concept OptionalType = isOptionalTypeV<T>;

// =============================================================================
// Smart Pointer Traits & Concepts
// =============================================================================

// Broad smart-pointer shape. Kept because some generic code only needs to know
// that a type is dereferenceable and nullable.
template <typename T>
concept SmartPointer = requires(T p) {
    typename T::element_type;
    p.get();
    static_cast<bool>(p);
    *p;
};

template <typename T>
struct IsSharedPointer : std::false_type {};

template <typename T>
struct IsSharedPointer<std::shared_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool isSharedPointerV = IsSharedPointer<std::remove_cvref_t<T>>::value;

template <typename T>
concept SharedPointer = isSharedPointerV<T>;

template <typename T>
struct IsUniquePointer : std::false_type {};

template <typename T, typename Deleter>
struct IsUniquePointer<std::unique_ptr<T, Deleter>> : std::true_type {};

template <typename T>
inline constexpr bool isUniquePointerV = IsUniquePointer<std::remove_cvref_t<T>>::value;

template <typename T>
concept UniquePointer = isUniquePointerV<T>;

template <typename T>
struct IsWeakPointer : std::false_type {};

template <typename T>
struct IsWeakPointer<std::weak_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool isWeakPointerV = IsWeakPointer<std::remove_cvref_t<T>>::value;

template <typename T>
concept WeakPointer = isWeakPointerV<T>;

template <typename T>
concept OwningSmartPointer = SharedPointer<T> || UniquePointer<T>;

template <typename T>
concept UnsupportedPersistentPointer =
    std::is_pointer_v<std::remove_cvref_t<T>> || WeakPointer<T>;

// =============================================================================
// Extended Character Types
// =============================================================================

template <typename T>
concept ExtendedCharType =
    std::same_as<std::remove_cvref_t<T>, wchar_t>  ||
    std::same_as<std::remove_cvref_t<T>, char8_t>  ||
    std::same_as<std::remove_cvref_t<T>, char16_t> ||
    std::same_as<std::remove_cvref_t<T>, char32_t>;

// =============================================================================
// Fixed-Size Array Traits & Concept
// =============================================================================

template <typename T>
struct IsStdArray : std::false_type {};

template <typename T, std::size_t Size>
struct IsStdArray<std::array<T, Size>> : std::true_type {};

template <typename T>
inline constexpr bool isStdArrayV = IsStdArray<std::remove_cvref_t<T>>::value;

template <typename T>
concept StdArrayType = isStdArrayV<T>;

template <StdArrayType T>
inline constexpr std::size_t stdArraySizeV = std::tuple_size_v<std::remove_cvref_t<T>>;

// =============================================================================
// Container Concepts
// =============================================================================

// Broad container classification used by existing code.
template <typename T>
concept ReflectableContainer = requires(T a) {
    typename T::value_type;
    a.begin();
    a.end();
} && !std::same_as<std::remove_cvref_t<T>, std::string> && !OptionalType<T>;

// Associative key/value container.
template <typename T>
concept MapContainer = requires(T a) {
    typename T::key_type;
    typename T::mapped_type;
    typename T::value_type;
    a.begin();
    a.end();
} && ReflectableContainer<T>;

// Associative single-value container such as std::set / std::unordered_set.
template <typename T>
concept SetContainer = requires(T a) {
    typename T::key_type;
    typename T::value_type;
    a.begin();
    a.end();
} && ReflectableContainer<T> && !MapContainer<T>;

// Fixed-size sequence container.
template <typename T>
concept FixedSequenceContainer = StdArrayType<T>;

// Mutable sequence container supporting clear + push_back.
template <typename T>
concept PushBackSequenceContainer =
    ReflectableContainer<T> &&
    !MapContainer<T> &&
    !SetContainer<T> &&
    !FixedSequenceContainer<T> &&
    requires(T a, typename T::value_type value) {
        a.clear();
        a.push_back(std::move(value));
    };

// Mutable sequence container supporting clear + insert.
template <typename T>
concept InsertSequenceContainer =
    ReflectableContainer<T> &&
    !MapContainer<T> &&
    !FixedSequenceContainer<T> &&
    requires(T a, typename T::value_type value) {
        a.clear();
        a.insert(std::move(value));
    };

// Any container category for which generic persistence reconstruction is known.
template <typename T>
concept PersistentContainer = MapContainer<T> || FixedSequenceContainer<T> || PushBackSequenceContainer<T> || InsertSequenceContainer<T>;

// =============================================================================
// Signal Traits & Concept
// =============================================================================

template <typename T>
struct IsSignal : std::false_type {};

template <typename... Args>
struct IsSignal<Signal<Args...>> : std::true_type {};

template <typename T>
inline constexpr bool isSignalTypeV = IsSignal<std::remove_cvref_t<T>>::value;

template <typename T>
concept SignalType = isSignalTypeV<T>;

template <typename T>
concept SignalObjectType = LightObjectType<T> || ObjectType<T>;

// =============================================================================
// Member Pointer Traits
// =============================================================================

template <typename T>
struct MemberObjectPointerTraits;

template <typename Member, typename Owner>
struct MemberObjectPointerTraits<Member Owner::*> {
    using MemberType = Member;
    using OwnerType  = Owner;
};

template <typename T>
struct MemberFunctionPointerTraits;

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...)> {
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) const> {
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) noexcept> {
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) const noexcept> {
    using ReturnType = Return;
    using OwnerType  = Owner;
};

// =============================================================================
// Reflection Helpers
// =============================================================================

consteval void appendReflectedDataMembers(std::vector<std::meta::info> &members, std::meta::info type)
{
    constexpr auto ctx = std::meta::access_context::unchecked();

    for (const auto base : std::meta::bases_of(type, ctx))
        appendReflectedDataMembers(members, std::meta::type_of(base));

    for (const auto member : std::meta::nonstatic_data_members_of(type, ctx))
        members.push_back(member);
}

template <typename T>
consteval auto getReflectedDataMembers()
{
    std::vector<std::meta::info> members;
    appendReflectedDataMembers(members, ^^T);
    return std::define_static_array(members);
}

template <typename T>
inline constexpr auto reflectedDataMembersV = getReflectedDataMembers<T>();

consteval void appendReflectedMembers(std::vector<std::meta::info> &members, std::meta::info type)
{
    constexpr auto ctx = std::meta::access_context::unchecked();

    for (const auto base : std::meta::bases_of(type, ctx))
        appendReflectedMembers(members, std::meta::type_of(base));

    for (const auto member : std::meta::members_of(type, ctx))
        members.push_back(member);
}

template <typename T>
consteval auto getReflectedMembers()
{
    std::vector<std::meta::info> members;
    appendReflectedMembers(members, ^^T);
    return std::define_static_array(members);
}

template <typename T>
inline constexpr auto reflectedMembersV = getReflectedMembers<T>();

// =============================================================================
// Slot Evaluation Helpers
// =============================================================================

consteval bool isSlot(std::meta::info member)
{
    if (!std::meta::is_function(member))
        return false;

    if (!std::meta::has_identifier(member))
        return false;

    const auto name = std::meta::identifier_of(member);
    return name.starts_with("slot") || name.starts_with("handle");
}

template <typename T>
consteval std::meta::info findSlot(std::string_view name)
{
    template for (constexpr auto member : reflectedMembersV<T>) {
        if constexpr (isSlot(member)) {
            if (std::meta::identifier_of(member) == name)
                return member;
        }
    }

    return {};
}

template <typename T>
consteval bool hasSlot(std::string_view name)
{
    return findSlot<T>(name) != std::meta::info{};
}

template <typename T>
consteval std::size_t slotCount()
{
    std::size_t count = 0;

    template for (constexpr auto member : reflectedMembersV<T>) {
        if constexpr (isSlot(member))
            ++count;
    }

    return count;
}

} // namespace job::core
