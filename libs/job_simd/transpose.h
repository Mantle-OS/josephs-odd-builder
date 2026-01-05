#pragma once

#include <simd_provider.h>
namespace job::simd {

// 1 use the SIMD::width() OR us #if defined(__AVX512F__)
// FIXME transpose_kernel_8x16 or make this transpose_kernel_8xX where we look at SIMD::width() from the other headers

__attribute__((always_inline))
inline void transpose_kernel_8x8(const float* __restrict__ src, size_t src_stride,
                     float* __restrict__ dst, size_t dst_stride)
{
    // load 8 rows
    auto r0 = SIMD::pull(src + 0 * src_stride);
    auto r1 = SIMD::pull(src + 1 * src_stride);
    auto r2 = SIMD::pull(src + 2 * src_stride);
    auto r3 = SIMD::pull(src + 3 * src_stride);
    auto r4 = SIMD::pull(src + 4 * src_stride);
    auto r5 = SIMD::pull(src + 5 * src_stride);
    auto r6 = SIMD::pull(src + 6 * src_stride);
    auto r7 = SIMD::pull(src + 7 * src_stride);

    // merge 32 bit rows
    auto t0 = SIMD::unpack_lo(r0, r1);
    auto t1 = SIMD::unpack_hi(r0, r1);
    auto t2 = SIMD::unpack_lo(r2, r3);
    auto t3 = SIMD::unpack_hi(r2, r3);
    auto t4 = SIMD::unpack_lo(r4, r5);
    auto t5 = SIMD::unpack_hi(r4, r5);
    auto t6 = SIMD::unpack_lo(r6, r7);
    auto t7 = SIMD::unpack_hi(r6, r7);

    // shuffle 64-bit (swap 2-float chunks)
    // mask 1,0,1,0 -> 0x44 | 3,2,3,2 -> 0xEE
    auto u0 = SIMD::shuffle<0x44>(t0, t2);
    auto u1 = SIMD::shuffle<0xEE>(t0, t2);
    auto u2 = SIMD::shuffle<0x44>(t1, t3);
    auto u3 = SIMD::shuffle<0xEE>(t1, t3);
    auto u4 = SIMD::shuffle<0x44>(t4, t6);
    auto u5 = SIMD::shuffle<0xEE>(t4, t6);
    auto u6 = SIMD::shuffle<0x44>(t5, t7);
    auto u7 = SIMD::shuffle<0xEE>(t5, t7);

    // permute 128-bit lanes (final swap)
    // 0x20: low 128 of a, low 128 of b
    // 0x31: high 128 of a, high 128 of b
    SIMD::mov(dst + 0 * dst_stride, SIMD::permute_lanes<0x20>(u0, u4));
    SIMD::mov(dst + 1 * dst_stride, SIMD::permute_lanes<0x20>(u1, u5));
    SIMD::mov(dst + 2 * dst_stride, SIMD::permute_lanes<0x20>(u2, u6));
    SIMD::mov(dst + 3 * dst_stride, SIMD::permute_lanes<0x20>(u3, u7));
    SIMD::mov(dst + 4 * dst_stride, SIMD::permute_lanes<0x31>(u0, u4));
    SIMD::mov(dst + 5 * dst_stride, SIMD::permute_lanes<0x31>(u1, u5));
    SIMD::mov(dst + 6 * dst_stride, SIMD::permute_lanes<0x31>(u2, u6));
    SIMD::mov(dst + 7 * dst_stride, SIMD::permute_lanes<0x31>(u3, u7));
}

__attribute__((always_inline))
inline void transpose_kernel_4x4(const float* __restrict__ src, size_t src_stride,
                     float* __restrict__ dst, size_t dst_stride)
{
    // 4 rows
    auto r0 = SIMD::pull(src + 0 * src_stride);
    auto r1 = SIMD::pull(src + 1 * src_stride);
    auto r2 = SIMD::pull(src + 2 * src_stride);
    auto r3 = SIMD::pull(src + 3 * src_stride);

    // 4x4 transpose (SSE/NEON/AVX)
    auto t0 = SIMD::unpack_lo(r0, r1); // [00 10 01 11]
    auto t1 = SIMD::unpack_hi(r0, r1); // [02 12 03 13]
    auto t2 = SIMD::unpack_lo(r2, r3); // [20 30 21 31]
    auto t3 = SIMD::unpack_hi(r2, r3); // [22 32 23 33]

    SIMD::mov(dst + 0 * dst_stride, SIMD::shuffle<0x44>(t0, t2)); // [00 10 20 30]
    SIMD::mov(dst + 1 * dst_stride, SIMD::shuffle<0xEE>(t0, t2)); // [01 11 21 31]
    SIMD::mov(dst + 2 * dst_stride, SIMD::shuffle<0x44>(t1, t3)); // [02 12 22 32]
    SIMD::mov(dst + 3 * dst_stride, SIMD::shuffle<0xEE>(t1, t3)); // [03 13 23 33]
}

inline void transpose(const float* __restrict__ src, float* __restrict__ dst, int rows, int cols) {
    constexpr int K = SIMD::width(); // Note this could be 16 in the future of avx-512
    int i = 0;
    for (; i + (K-1) < rows; i += K) {
        int j = 0;
        for (; j + (K-1) < cols; j += K)
            // ADD 16 // transpose_kernel_XxX for 512 ?
            if constexpr (K == 8)
                transpose_kernel_8x8(src + i * cols + j, cols, dst + j * rows + i, rows);
            else
                transpose_kernel_4x4(src + i * cols + j, cols, dst + j * rows + i, rows);

        // scalar fallback for remaining columns
        for (; j < cols; ++j)
            for (int k = 0; k < K; ++k)
                dst[j * rows + (i + k)] = src[(i + k) * cols + j];
    }

    // scalar fallback for remaining rows
    for (; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            dst[j * rows + i] = src[i * cols + j];
}

} // namespace job::simd
