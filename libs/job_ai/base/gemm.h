#pragma once

#include "real_type.h"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <immintrin.h>
#include <job_thread_pool.h>
#include <job_parallel_for.h>

namespace job::ai::base {

// =============================================================================
//  CONFIGURATION
// =============================================================================

// [Server] Ryzen 5950X (Use 256 for release builds on server)
// constexpr int kBlockSize = 256;

// [Laptop] Ryzen 5900HX - Safer default for thermal/cache constraints
constexpr int kBlockSize = 128;

constexpr int kMicroM = 8;
constexpr int kMicroN = 8;

// =============================================================================
//  MICRO KERNEL (8x8)
// =============================================================================
// Computes 8x8 block: C = alpha * (A * B) + C
__attribute__((always_inline))
inline void microKernel8x8(int K, core::real_t alpha,
               const core::real_t* __restrict__ A, int lda,
               const core::real_t* __restrict__ B, int ldb,
               core::real_t* __restrict__ C, int ldc)
{
    __m256 c0 = _mm256_setzero_ps();
    __m256 c1 = _mm256_setzero_ps();
    __m256 c2 = _mm256_setzero_ps();
    __m256 c3 = _mm256_setzero_ps();
    __m256 c4 = _mm256_setzero_ps();
    __m256 c5 = _mm256_setzero_ps();
    __m256 c6 = _mm256_setzero_ps();
    __m256 c7 = _mm256_setzero_ps();

    for (int p = 0; p < K; ++p) {
        __m256 b_row = _mm256_loadu_ps(B + p * ldb);
        c0 = _mm256_fmadd_ps(_mm256_set1_ps(A[0 * lda + p]), b_row, c0);
        c1 = _mm256_fmadd_ps(_mm256_set1_ps(A[1 * lda + p]), b_row, c1);
        c2 = _mm256_fmadd_ps(_mm256_set1_ps(A[2 * lda + p]), b_row, c2);
        c3 = _mm256_fmadd_ps(_mm256_set1_ps(A[3 * lda + p]), b_row, c3);
        c4 = _mm256_fmadd_ps(_mm256_set1_ps(A[4 * lda + p]), b_row, c4);
        c5 = _mm256_fmadd_ps(_mm256_set1_ps(A[5 * lda + p]), b_row, c5);
        c6 = _mm256_fmadd_ps(_mm256_set1_ps(A[6 * lda + p]), b_row, c6);
        c7 = _mm256_fmadd_ps(_mm256_set1_ps(A[7 * lda + p]), b_row, c7);
    }

    __m256 alpha_vec = _mm256_set1_ps(alpha);

#define UPDATE_C(reg, row_idx) \
    reg = _mm256_fmadd_ps(reg, alpha_vec, _mm256_loadu_ps(C + (row_idx) * ldc)); \
        _mm256_storeu_ps(C + (row_idx) * ldc, reg)

        UPDATE_C(c0, 0); UPDATE_C(c1, 1); UPDATE_C(c2, 2); UPDATE_C(c3, 3);
    UPDATE_C(c4, 4); UPDATE_C(c5, 5); UPDATE_C(c6, 6); UPDATE_C(c7, 7);
#undef UPDATE_C
}

// =============================================================================
//  INTERNAL: Compute One Block
// =============================================================================
// Helper to compute a rectangular block starting at (start_m, start_n)
inline void compute_tile(int M, int N, int K, core::real_t alpha,
                         const core::real_t* A, int lda,
                         const core::real_t* B, int ldb,
                         core::real_t* C, int ldc,
                         int start_m, int start_n, int block_size)
{
    int M_curr = std::min(block_size, M - start_m);
    int N_curr = std::min(block_size, N - start_n);

    for (int mi = 0; mi < M_curr; mi += kMicroM) {
        for (int ni = 0; ni < N_curr; ni += kMicroN) {

            if (mi + kMicroM <= M_curr && ni + kMicroN <= N_curr) {
                microKernel8x8(K, alpha,
                               &A[(start_m + mi) * lda], lda,
                               &B[(start_n + ni)], ldb,
                               &C[(start_m + mi) * ldc + (start_n + ni)], ldc);
            } else {
                // Scalar Fallback for Edges
                int m_limit = std::min(kMicroM, M_curr - mi);
                int n_limit = std::min(kMicroN, N_curr - ni);
                for (int row = 0; row < m_limit; ++row) {
                    for (int col = 0; col < n_limit; ++col) {
                        core::real_t sum = 0.0f;
                        for (int p = 0; p < K; ++p) {
                            sum += A[(start_m + mi + row) * lda + p] * B[p * ldb + (start_n + ni + col)];
                        }
                        C[(start_m + mi + row) * ldc + (start_n + ni + col)] += alpha * sum;
                    }
                }
            }
        }
    }
}

// =============================================================================
//  SERIAL SGEMM
// =============================================================================
inline void sgemm(int M, int N, int K,
                  core::real_t alpha, const core::real_t* A, int lda,
                  const core::real_t* B, int ldb,
                  core::real_t beta, core::real_t* C, int ldc)
{
    // Beta Scaling
    if (beta == 0.0f) {
        for(int i=0; i<M; ++i) std::memset(C + i * ldc, 0, N * sizeof(core::real_t));
    } else if (beta != 1.0f) {
        for(int i=0; i<M; ++i)
            for(int j=0; j<N; ++j) C[i * ldc + j] *= beta;
    }

    // Tiling Loop
    for (int i = 0; i < M; i += kBlockSize) {
        for (int j = 0; j < N; j += kBlockSize) {
            compute_tile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, kBlockSize);
        }
    }
}

//  PARALLEL SGEMM (2D Tiled)
// Parallelizes over tiles (M chunks * N chunks) to ensure tasks are small
// enough to prevent watchdog timeouts, even in Debug mode.
inline void sgemm_parallel(job::threads::ThreadPool &pool,
                           int M, int N, int K,
                           core::real_t alpha, const core::real_t* A, int lda,
                           const core::real_t* B, int ldb,
                           core::real_t beta, core::real_t* C, int ldc)
{
    // beta Scaling (Row Parallel)
    job::threads::parallel_for(pool, size_t{0}, size_t(M), [&](size_t i) {
        if (beta == 0.0f) {
            std::memset(C + i * ldc, 0, N * sizeof(core::real_t));
        } else if (beta != 1.0f) {
            for(int j=0; j<N; ++j) C[i * ldc + j] *= beta;
        }
    });

    // 2. 2D Block Parallelization
    int m_chunks = (M + kBlockSize - 1) / kBlockSize;
    int n_chunks = (N + kBlockSize - 1) / kBlockSize;
    size_t total_tiles = m_chunks * n_chunks;

    job::threads::parallel_for(pool, size_t{0}, total_tiles, [&](size_t tile_idx) {
        int chunk_m = tile_idx / n_chunks;
        int chunk_n = tile_idx % n_chunks;

        int i = chunk_m * kBlockSize;
        int j = chunk_n * kBlockSize;

        compute_tile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, kBlockSize);
    });
}

// =============================================================================
//  STRIDED BATCHED (Already Parallel)
// =============================================================================
inline void sgemm_strided_batched(job::threads::ThreadPool& pool,
                                  int batchCount,
                                  int M, int N, int K,
                                  core::real_t alpha,
                                  const core::real_t* A, int strideA, int lda,
                                  const core::real_t* B, int strideB, int ldb,
                                  core::real_t beta,
                                  core::real_t* C, int strideC, int ldc)
{
    job::threads::parallel_for(pool, size_t{0}, size_t(batchCount), [&](size_t i) {
        const core::real_t* a_ptr = A + (i * strideA);
        const core::real_t* b_ptr = B + (i * strideB);
        core::real_t* c_ptr       = C + (i * strideC);
        sgemm(M, N, K, alpha, a_ptr, lda, b_ptr, ldb, beta, c_ptr, ldc);
    });
}

} // namespace job::science::la
