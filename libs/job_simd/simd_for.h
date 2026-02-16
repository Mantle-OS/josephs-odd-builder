#pragma once

#include <utility>
#include <algorithm>

#include <job_parallel_for.h>
#include <simd_provider.h>

namespace job::simd {

//
// Generic SIMD Loop
// kWidth = SIMD width (8 for AVX2, 16 for AVX512)
// vFunc: Lambda called with index 'i' for vector steps (increments by kWidth)
// sFunc: Lambda called with index 'i' for scalar tail (increments by 1)


// serial
template<typename VecFunc, typename ScalarFunc>
__attribute__((always_inline))
inline void simd_for(size_t start, size_t end, VecFunc &&vFunc, ScalarFunc &&sFunc)
{
    constexpr size_t kWidth = SIMD::width();
    size_t i = start;

    // Life in the fast lane ... surely make you lose your mind.
    for (; i + (kWidth - 1) < end; i += kWidth)
        vFunc(i);


    // Picking up the scraps (tail).
    for (; i < end; ++i)
        sFunc(i);
}

template<typename VecFunc, typename ScalarFunc>
__attribute__((always_inline))
inline void simd_for(size_t size, VecFunc &&vFunc, ScalarFunc &&sFunc)
{
    simd_for(0, size, std::forward<VecFunc>(vFunc), std::forward<ScalarFunc>(sFunc));
}

// parallel_for
template<typename VecFunc, typename ScalarFunc>
__attribute__((always_inline))
inline void simd_for(job::threads::ThreadPool &pool,
                     size_t start,
                     size_t end,
                     VecFunc &&vFunc,
                     ScalarFunc &&sFunc)
{
    if (start >= end)
        return;

    constexpr size_t kMinGrain = 1024;
    size_t count = end - start;

    size_t hw = pool.workerCount();
    if (hw == 0)
        hw = 1;

    if (count < kMinGrain || hw == 1) {
        simd_for(start, end, std::forward<VecFunc>(vFunc), std::forward<ScalarFunc>(sFunc));
        return;
    }

    size_t chunkSize = std::max(kMinGrain, (count + hw - 1) / hw);
    size_t numChunks = (count + chunkSize - 1) / chunkSize;

    job::threads::parallel_for(pool, size_t{0}, numChunks, [&](size_t chunkIdx) {
        size_t chunkStart = start + (chunkIdx * chunkSize);
        size_t chunkEnd   = std::min(chunkStart + chunkSize, end);

        simd_for(chunkStart, chunkEnd, vFunc, sFunc);
    });
}

// Convenience overload for 0 to N (Parallel)
template<typename VecFunc, typename ScalarFunc>
__attribute__((always_inline))
inline void simd_for(job::threads::ThreadPool &pool,
                     size_t size,
                     VecFunc &&vFunc,
                     ScalarFunc &&sFunc)
{
    simd_for(pool, 0, size, std::forward<VecFunc>(vFunc), std::forward<ScalarFunc>(sFunc));
}

} // namespace job::simd
