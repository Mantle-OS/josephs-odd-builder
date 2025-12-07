#pragma once

#include <vector>
#include <random>
#include <memory>
#include <algorithm>
#include <cstdint>

#include "aligned_allocator.h"

#include "simd_provider.h"

namespace job::ai::comp {

class NoiseTable final {
public:

#ifdef JOB_BLOCK_SIZE_256
    // 32MB = 8 Million floats. Fits in 64MB L3 Cache.
    static constexpr size_t kSize = 8 * 1024 * 1024;
#else
    // 16MB = 4 Million floats. Safer for Laptop/ARM L3 sizes.
    static constexpr size_t kSize = 4 * 1024 * 1024;
#endif

    static constexpr size_t kMask = kSize - 1; // For fast wrapping
    static_assert((kSize & kMask) == 0, "kSize must be a power-of-two.");

    static NoiseTable& instance()
    {
        static NoiseTable inst;
        return inst;
    }

    // w_ptr: Pointer to weights to mutate (should be aligned).
    // count: Number of weights
    // seed:  Unique mutation ID (generation + thread_id)
    // sigma: Mutation strength (learning rate)
    void perturb(float* __restrict__ w_ptr, size_t count, uint64_t seed, float sigma) const
    {


        size_t offset = (seed * 0x9E3779B97F4A7C15ULL) & kMask;

        const float* __restrict__ n_ptr = m_data.data();
        auto vSigma = SIMD::set1(sigma); // Broadcast sigma
        constexpr size_t W = SIMD::width();

        size_t i = 0;

        for (; i + (W - 1) < count; i += W) {
            size_t idx = (offset + i) & kMask;

            // Fast Path: Contiguous load (No wrapping in the middle of a register)
            if (idx + W <= kSize) {
                auto w = SIMD::pull(w_ptr + i);     // Load Weights
                auto n = SIMD::pull(n_ptr + idx);   // Load Noise
                w = SIMD::mul_plus(n, vSigma, w);   // w = w + (n * sigma)
                SIMD::mov(w_ptr + i, w);            // Store back
            }
            // Slow Path: The wrap happens inside this register block
            else {
                for (size_t k = 0; k < W; ++k) {
                    size_t wrap_idx = (idx + k) & kMask;
                    w_ptr[i + k] += n_ptr[wrap_idx] * sigma;
                }
            }
        }
        for (; i < count; ++i) {
            size_t idx = (offset + i) & kMask;
            w_ptr[i] += n_ptr[idx] * sigma;
        }
    }

    [[nodiscard]] const float* data() const { return m_data.data(); }
    [[nodiscard]] size_t size() const { return kSize; }

private:
    NoiseTable() {
        m_data.resize(kSize);
        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, 1.0f);

        for(auto& f : m_data)
            f = dist(gen);
    }

    // Aligned storage for SIMD loads
    std::vector<float, job::ai::cords::AlignedAllocator<float, 64>> m_data;
};

} // namespace job::ai::comp
