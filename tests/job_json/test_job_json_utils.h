#pragma once

#include <chrono>
#include <cstddef>
#include <utility>

#include "../tests-fast-math-workaround.h"

namespace job::json::tests {

template <typename T>
[[nodiscard]] inline T benchmarkOpaque(T value) noexcept
{
    asm volatile("" : "+r"(value) : : "memory");
    return value;
}

template <typename T>
inline void benchmarkDoNotOptimize(const T &value) noexcept
{
    asm volatile("" : : "g"(&value) : "memory");
}

template <typename T>
inline void benchmarkClobber(T &value) noexcept
{
    asm volatile("" : "+m"(value) : : "memory");
}

inline void benchmarkClobberMemory() noexcept
{
    asm volatile("" : : : "memory");
}

template <typename Function>
[[nodiscard]] double benchmarkNanosecondsPerIteration(
    std::size_t iterations,
    Function &&function)
{
    const auto begin = std::chrono::steady_clock::now();

    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        std::forward<Function>(function)();
        benchmarkClobberMemory();
    }

    const auto end = std::chrono::steady_clock::now();

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return static_cast<double>(elapsed) / static_cast<double>(iterations);
}

} // namespace job::json::tests