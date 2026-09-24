#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace job::core {

template<typename T>
concept JobObject = requires(const T &object)
{
    { object.uid() } -> std::convertible_to<std::string_view>;
};

template<typename T>
struct IsJobObjectPointer : std::false_type
{
};

template<typename T>
struct IsJobObjectPointer<T *> : std::true_type
{
};

template<typename T>
struct IsJobObjectPointer<std::shared_ptr<T>> : std::true_type
{
};

template<typename T>
struct IsJobObjectPointer<std::unique_ptr<T>> : std::true_type
{
};

template<typename T>
concept JobObjectPointer = IsJobObjectPointer<T>::value;

} // namespace job::core