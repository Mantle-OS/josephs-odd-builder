#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "obj.h"
#include "signal.h"


//
// Object
//

template <typename T>
concept ObjectType = std::derived_from<T, Object>;

//
// Signal type detection
//

template <typename T>
struct IsSignal : std::false_type
{

};

template <typename... Args>
struct IsSignal<Signal<Args...>> : std::true_type
{

};

template <typename T>
inline constexpr bool isSignalTypeV = IsSignal<std::remove_cvref_t<T>>::value;

template <typename T>
concept SignalType = isSignalTypeV<T>;

//
// Member pointer information
//

template <typename T>
struct MemberObjectPointerTraits;

template <typename Member, typename Owner>
struct MemberObjectPointerTraits<Member Owner::*>
{
    using MemberType = Member;
    using OwnerType  = Owner;
};


template <typename T>
struct MemberFunctionPointerTraits;

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...)>
{
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) const>
{
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) noexcept>
{
    using ReturnType = Return;
    using OwnerType  = Owner;
};

template <typename Return, typename Owner, typename... Args>
struct MemberFunctionPointerTraits<Return (Owner::*)(Args...) const noexcept>
{
    using ReturnType = Return;
    using OwnerType  = Owner;
};


//
// Reflection
//

template <ObjectType T>
consteval auto reflectedMembers()
{
    constexpr auto ctx = std::meta::access_context::unchecked();
    return std::define_static_array(std::meta::members_of(^^T, ctx));
}


//
// Slot reflection
//
// For now a slot is:
//
//   1. a function
//   2. with an identifier
//   3. whose identifier begins with "slot" OR "handle"
//
// Eventually this can become annotation based.
//
consteval bool isSlot(std::meta::info member)
{
    if (!std::meta::is_function(member))
        return false;

    if (!std::meta::has_identifier(member))
        return false;

    const auto name = std::meta::identifier_of(member);

    return
        name.starts_with("slot") ||
        name.starts_with("handle");
}


consteval std::string_view memberName(std::meta::info member)
{
    if (!std::meta::has_identifier(member))
        return {};

    return std::meta::identifier_of(member);
}


template <ObjectType T>
consteval std::meta::info findSlot(std::string_view name)
{
    template for (constexpr auto member : reflectedMembers<T>()) {
        if constexpr (isSlot(member)) {
            if (std::meta::identifier_of(member) == name)
                return member;
        }
    }

    return {};
}

template <ObjectType T>
consteval bool hasSlot(std::string_view name)
{
    return findSlot<T>(name) != std::meta::info{};
}


template <ObjectType T>
consteval std::size_t slotCount()
{
    std::size_t count = 0;
    template for (constexpr auto member : reflectedMembers<T>()){
        if constexpr (isSlot(member))
            ++count;
    }

    return count;
}


consteval std::size_t parameterCount(std::meta::info function)
{
    if (!std::meta::is_function(function))
        return 0;

    return
        std::meta::parameters_of(function).size();
}


template <ObjectType T>
consteval bool slotHasParameterCount(std::string_view name, std::size_t count)
{
    const auto slot = findSlot<T>(name);
    if (slot == std::meta::info{})
        return false;

    return
        parameterCount(slot) == count;
}


//
// Signal connection implementation
//
// This specialization cracks Signal<Args...> open so the compiler
// can validate the slot against the exact emitted argument list.
//

template <typename T>
struct SignalConnection;

template <typename... Args>
struct SignalConnection<Signal<Args...>>
{
    template <auto SlotMember, ObjectType Receiver>
    static void bind(Signal<Args...>& signal, Receiver& receiver)
    {
        static_assert(std::is_member_function_pointer_v<decltype(SlotMember)>,
                      "Slot must be a member function");

        static_assert(std::is_invocable_v<decltype(SlotMember), Receiver&, Args...>,
                      "Signal arguments are not compatible with slot");

        const auto connectionId = signal.connect([&receiver](Args... args) {
            std::invoke(SlotMember, receiver, std::forward<Args>(args)...);
        });

        receiver.registerConnection([&signal, connectionId]() {
            signal.disconnect(connectionId);
        });
    }
};

//
// Connection
//
// Usage:
//
//     connect<
//         &Ping::pingChanged,
//         &Pong::handlePing
//     >(ping, pong);
//
// SignalMember and SlotMember are compile-time identities.
// sender and receiver are runtime objects.
//

template <auto SignalMember, auto SlotMember, ObjectType Sender, ObjectType Receiver>
void connect(Sender& sender, Receiver& receiver)
{
    static_assert(std::is_member_object_pointer_v<decltype(SignalMember)>,
                  "Signal must be a member object");

    static_assert(std::is_member_function_pointer_v<decltype(SlotMember)>,
                  "Slot must be a member function");

    using SignalMemberInfo  = MemberObjectPointerTraits<decltype(SignalMember)>;
    using SignalOwner       = typename SignalMemberInfo::OwnerType;
    using SignalT           = typename SignalMemberInfo::MemberType;
    using SlotMemberInfo    = MemberFunctionPointerTraits<decltype(SlotMember)>;
    using SlotOwner         = typename SlotMemberInfo::OwnerType;

    static_assert(std::derived_from<Sender, SignalOwner>,
                  "Signal does not belong to sender object");

    static_assert(std::derived_from<Receiver, SlotOwner>,
                  "Slot does not belong to receiver object");

    static_assert(SignalType<SignalT>,
                  "Selected member is not a Signal");

    auto &signal = sender.*SignalMember;
    SignalConnection<std::remove_cvref_t<SignalT>>::template bind<SlotMember>(signal, receiver);
}