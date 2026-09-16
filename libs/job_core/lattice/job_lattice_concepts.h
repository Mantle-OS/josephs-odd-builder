#pragma once

#include <concepts>
#include <type_traits>

namespace job::core::lattice {

template <typename T>
concept LatticeCoordinateType = requires(T value) {
    requires std::same_as<std::remove_cvref_t<decltype(value.x)>, float>;
    requires std::same_as<std::remove_cvref_t<decltype(value.y)>, float>;
    requires std::same_as<std::remove_cvref_t<decltype(value.z)>, float>;
    requires std::same_as<std::remove_cvref_t<decltype(value.m)>, float>;
};

template <typename T>
concept LatticePointType =requires(T value) {
    requires LatticeCoordinateType<
        std::remove_cvref_t<decltype(value.coordinate)>>;
};

} // namespace job::core::lattice