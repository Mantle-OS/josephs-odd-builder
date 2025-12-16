#pragma once
#include <algorithm>

#include <job_parallel_for.h>

#include <simd_provider.h>

#include "activation_functors.h"
#include "activation_types.h"
#include "activation_math.h"


namespace job::ai::comp {
using namespace job::simd;

template<typename Op, bool IsVectorizable>
inline void activation_kernel_impl(float* __restrict__ data, size_t n, float alpha)
{
    size_t i = 0;
    if constexpr (IsVectorizable) {
        constexpr int K = SIMD::width();
        for (; i + (K-1) < n; i += K) {
            auto v = SIMD::pull(data + i);
            // 2. Pass alpha to the vector op
            v = Op::apply(v, alpha);
            SIMD::mov(data + i, v);
        }
    }
    for (; i < n; ++i)
        // 3. Pass alpha to the scalar op
        data[i] = Op::apply_s(data[i], alpha);
}
inline void activate_buffer([[maybe_unused]]job::threads::ThreadPool &pool, float *data, size_t n, ActivationType type, float alpha = 0.0f)
{
    // constexpr size_t kMinParallelSize = 16384;
    constexpr size_t kMinParallelSize = 262144;
    if (n < kMinParallelSize) {
        switch (type) {
        case ActivationType::Identity:
            break;
        case ActivationType::ReLU:
            activation_kernel_impl<FunctorReLU, true>(data, n, alpha );
            break;
        case ActivationType::LeakyReLU:
            activation_kernel_impl<FunctorLeakyReLU, true>(data, n, alpha );
            break;
        case ActivationType::Swish:
            activation_kernel_impl<FunctorSwish, true>(data, n, alpha );
            break;
        case ActivationType::GELU:
            activation_kernel_impl<FunctorGELU, true>(data, n, alpha );
            break;
        default:
            for (size_t i = 0; i < n; ++i)
                data[i] = activate(type, data[i], alpha);
            break;
        }
        return;
    }

    constexpr size_t kTileSize = 1024;
    size_t num_tiles = (n + kTileSize - 1) / kTileSize;

    job::threads::parallel_for(pool, size_t{0}, num_tiles, [=](size_t tile_idx) {
        size_t start = tile_idx * kTileSize;
        size_t end = std::min(start + kTileSize, n);
        size_t len = end - start;
        float *ptr = data + start;

        // The Switch happens ONCE per Tile (not per float)
        switch (type) {
        case ActivationType::Identity:
            break;
        case ActivationType::ReLU:
            activation_kernel_impl<FunctorReLU, true>(ptr, len, alpha );
            break;
        case ActivationType::LeakyReLU:
            activation_kernel_impl<FunctorLeakyReLU, true>(ptr, len, alpha);
            break;
        case ActivationType::Swish:
            activation_kernel_impl<FunctorSwish, true>(ptr, len, alpha );
            break;
        case ActivationType::GELU:
            activation_kernel_impl<FunctorGELU, true>(ptr, len, alpha );
            break;
        default:
            for (size_t i = 0; i < len; ++i)
                ptr[i] = activate(type, ptr[i], alpha);
            break;
        }
    });
}

} // namespace job::ai::comp
