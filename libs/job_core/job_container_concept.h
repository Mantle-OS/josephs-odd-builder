#pragma once

#include <concepts>
#include <cstddef>

namespace job::core {

template<typename T>
concept JobContainer = requires(T container, const T constContainer)
{
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;

    { constContainer.size() } -> std::convertible_to<std::size_t>;
    { constContainer.isEmpty() } -> std::convertible_to<bool>;

    { container.begin() };
    { container.end() };

    { constContainer.begin() };
    { constContainer.end() };

    { container.clear() };
};

}