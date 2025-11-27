#pragma once

#include <concepts>
#include <type_traits>

// Concepts: “do you even vector, bro?”

namespace job::threads {

// !!!!!! WARNING !!!!!!
// Vector must support v+v and v*scalar -> v

template <class V, class S>
concept VecOps = requires(V v, V u, S s)
{
    { v + u } -> std::same_as<V>;
    { v * s } -> std::same_as<V>;

    // Component access (read AND write)
    { v.x } -> std::same_as<std::remove_reference_t<decltype(v.x)>&>;
    { v.y } -> std::same_as<std::remove_reference_t<decltype(v.y)>&>;
    { v.z } -> std::same_as<std::remove_reference_t<decltype(v.z)>&>;

    // !!!!!!MUST!!!!!! be convertible to the scalar type This is also used for barnsNhut . . .why it is here.
    { v.x } -> std::convertible_to<S>;
    { v.y } -> std::convertible_to<S>;
    { v.z } -> std::convertible_to<S>;

};

// Scalar !!!!!MUST!!!!!! be floating point (good for time steps, masses etc.)
template <class S>
concept FloatScalar = std::floating_point<S>;

} // namespace job::threads
// CHECKPOINT v1.1
