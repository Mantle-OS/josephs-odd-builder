#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <meta>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace job::core {

class BaseObject;
class Object;

template <typename... Args>
class Signal;

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
// Container Concepts
// =============================================================================

template <typename T>
concept ReflectableContainer = requires(T a) {
    typename T::value_type;
    a.begin();
    a.end();
} && !std::same_as<std::remove_cvref_t<T>, std::string> && !OptionalType<T>;

template <typename T>
concept MapContainer = requires(T a) {
    typename T::key_type;
    typename T::mapped_type;
    a.begin();
    a.end();
};

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

consteval void appendReflectedDataMembers(std::vector<std::meta::info> &members,
                                          std::meta::info type)
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

consteval void appendReflectedMembers(std::vector<std::meta::info> &members,
                                      std::meta::info type)
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