#pragma once

#include <concepts>
#include <memory>
#include <meta>
#include <type_traits>

namespace job::model {

template <typename T>
concept SerObjectBase = std::is_class_v<T> &&
                        requires {
                            typename T::Ptr;
                            typename T::WPtr;
                            typename T::UPtr;
                        } &&
    std::same_as<typename T::Ptr, std::shared_ptr<T>> &&
    std::same_as<typename T::WPtr, std::weak_ptr<T>> &&
    std::same_as<typename T::UPtr, std::unique_ptr<T>> &&
    std::default_initializable<T> &&
    std::destructible<T> &&
    !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> &&
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T> &&
    requires {
        { T::createShared() } -> std::same_as<typename T::Ptr>;
        { T::createUniq() } -> std::same_as<typename T::UPtr>;
    };

template <typename T>
consteval auto serMembers()
{
    constexpr auto ctx = std::meta::access_context::unchecked();
    return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
}

template <typename T>
consteval bool hasSerMembers()
{
    return !serMembers<T>().empty();
}

template <typename T>
concept SerObject = SerObjectBase<T> && hasSerMembers<T>();

} // namespace job::model