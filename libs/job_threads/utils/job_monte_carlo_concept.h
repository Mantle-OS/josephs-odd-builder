#pragma once
#include <cstdint>
#include <concepts>
#include <type_traits>

namespace job::threads {

template <typename Func, typename T>
concept ReducerFunc = requires(Func f, T a, T b)
{
    { f(a, b) } -> std::same_as<T>;
};

template <typename Func, typename T>
concept SimulatorFunc = requires(Func f, uint64_t index)
{
    { f(index) } -> std::same_as<T>;
};

}
