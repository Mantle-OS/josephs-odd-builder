#pragma once

#include <span>

namespace job::core::lattice {

template <typename T>
using LatticeView = std::span<T>;

template <typename T>
using ConstLatticeView = std::span<const T>;

} // namespace job::core::lattice