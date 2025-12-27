#pragma once
#include <concepts>
#include <real_type.h>

namespace job::science {

template <class V, class S>
concept VerletVector = core::FloatScalar<S> && requires(V v, V u, S s)

{
    // 3D indexing (read/write)
    { v[0] } -> std::convertible_to<S>;
    { v[1] } -> std::convertible_to<S>;
    { v[2] } -> std::convertible_to<S>;
    v[0] = s; v[1] = s; v[2] = s;

    // In-place ops are the "real" contract
    { v += u } -> std::same_as<V&>;
    { v -= u } -> std::same_as<V&>;
    { v *= s } -> std::same_as<V&>;

    // Allow scaled values to be accumulated (view * scalar can return a value)
    // This is the key relaxation.
    { u * s };
    requires requires(V w, decltype(u * s) t) {
        { w += t } -> std::same_as<V&>;
        { w -= t } -> std::same_as<V&>;
    };
};


} // namespace job::science
