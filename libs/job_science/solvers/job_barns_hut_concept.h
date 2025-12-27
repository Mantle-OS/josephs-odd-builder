#pragma once

#include <concepts>
#include <type_traits>

#include <real_type.h>

namespace job::science {

// !!!!!! WARNING !!!!!!
// Vector must support v+v and v*scalar -> v

template <class V, class S>
concept BarnesHutVector = requires(V v, V u, S s)
{
    { v + u } -> std::same_as<V>;
    { v * s } -> std::same_as<V>;

    // Component access (read AND write)
    { v.x } -> std::same_as<std::remove_reference_t<decltype(v.x)>&>;
    { v.y } -> std::same_as<std::remove_reference_t<decltype(v.y)>&>;
    { v.z } -> std::same_as<std::remove_reference_t<decltype(v.z)>&>;

    // !!!!!!MUST!!!!!! be convertible to the scalar type This is also used for barns N hut . . .why it is here.
    { v.x } -> std::convertible_to<S>;
    { v.y } -> std::convertible_to<S>;
    { v.z } -> std::convertible_to<S>;

};

}
