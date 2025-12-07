#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>

#include <job_thread_pool.h>
#include <job_parallel_for.h>

#include "simd_provider.h"

#include "matrix.h"

namespace job::ai::comp {
using namespace job::ai::cords;

using SMID = AVX_F;

constexpr int kMicroM = 8;
constexpr int kMicroN = 8;

//  MICRO KERNEL (8x8)
// Computes 8x8 block: C = alpha * (A * B) + C
__attribute__((always_inline))
inline void microKernel8x8(int K, float alpha,
               const float* __restrict__ A, int lda,
               const float* __restrict__ B, int ldb,
               float* __restrict__ C, int ldc)
{
    f32 c0 = SMID::zero();
    f32 c1 = SMID::zero();
    f32 c2 = SMID::zero();
    f32 c3 = SMID::zero();
    f32 c4 = SMID::zero();
    f32 c5 = SMID::zero();
    f32 c6 = SMID::zero();
    f32 c7 = SMID::zero();

    for (int p = 0; p < K; ++p) {
        f32 b_row = SMID::pull(B + p * ldb);
        c0 = SMID::mul_plus(SMID::set1(A[0 * lda + p]), b_row, c0);
        c1 = SMID::mul_plus(SMID::set1(A[1 * lda + p]), b_row, c1);
        c2 = SMID::mul_plus(SMID::set1(A[2 * lda + p]), b_row, c2);
        c3 = SMID::mul_plus(SMID::set1(A[3 * lda + p]), b_row, c3);
        c4 = SMID::mul_plus(SMID::set1(A[4 * lda + p]), b_row, c4);
        c5 = SMID::mul_plus(SMID::set1(A[5 * lda + p]), b_row, c5);
        c6 = SMID::mul_plus(SMID::set1(A[6 * lda + p]), b_row, c6);
        c7 = SMID::mul_plus(SMID::set1(A[7 * lda + p]), b_row, c7);
    }

    f32 alpha_vec = SMID::set1(alpha);

#define UPDATE_REG(reg, row_idx) \
    reg = SMID::mul_plus(reg, alpha_vec, SMID::pull(C + (row_idx) * ldc)); \
        SMID::mov(C + (row_idx) * ldc, reg)


    UPDATE_REG(c0, 0);
    UPDATE_REG(c1, 1);
    UPDATE_REG(c2, 2);
    UPDATE_REG(c3, 3);
    UPDATE_REG(c4, 4);
    UPDATE_REG(c5, 5);
    UPDATE_REG(c6, 6);
    UPDATE_REG(c7, 7);
#undef UPDATE_REG
}


inline void compute_tile(int M, int N, int K, float alpha,
                         const float *A, int lda,
                         const float *B, int ldb,
                         float *C, int ldc,
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
                        float sum = 0.0f;
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

// Low-level raw pointer sgemm (useful for batched operations on raw buffers)
inline void sgemm_raw(int M, int N, int K,
                      float alpha, const float *A, int lda,
                      const float *B, int ldb,
                      float beta, float *C, int ldc)
{
    // Beta Scaling
    if (beta == 0.0f) {
        for(int i=0; i<M; ++i) std::memset(C + i * ldc, 0, N * sizeof(float));
    } else if (beta != 1.0f) {
        for(int i=0; i<M; ++i)
            for(int j=0; j<N; ++j) C[i * ldc + j] *= beta;
    }

    // Tiling Loop
    for (int i = 0; i < M; i += JOB_BLOCK_SIZE)
        for (int j = 0; j < N; j += JOB_BLOCK_SIZE)
            compute_tile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, JOB_BLOCK_SIZE);
}

// Low-level raw pointer parallel sgemm
inline void sgemm_parallel_raw(job::threads::ThreadPool& pool,
                               int M, int N, int K,
                               float alpha, const float *A, int lda,
                               const float *B, int ldb,
                               float beta, float *C, int ldc)
{
    // Beta Scaling (Row Parallel)
    job::threads::parallel_for(pool, size_t{0}, size_t(M), [&](size_t i) {
        if (beta == 0.0f) {
            std::memset(C + i * ldc, 0, N * sizeof(float));
        } else if (beta != 1.0f) {
            for(int j=0; j<N; ++j)
                C[i * ldc + j] *= beta;
        }
    });

    // 2D Block Parallelization
    int m_chunks = (M + JOB_BLOCK_SIZE - 1) / JOB_BLOCK_SIZE;
    int n_chunks = (N + JOB_BLOCK_SIZE - 1) / JOB_BLOCK_SIZE;
    size_t total_tiles = m_chunks * n_chunks;

    job::threads::parallel_for(pool, size_t{0}, total_tiles, [&](size_t tile_idx) {
        int chunk_m = tile_idx / n_chunks;
        int chunk_n = tile_idx % n_chunks;
        int i = chunk_m * JOB_BLOCK_SIZE;
        int j = chunk_n * JOB_BLOCK_SIZE;

        compute_tile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, JOB_BLOCK_SIZE);
    }, 0, 1); // Grain size 1 to prevent timeouts
}

inline void sgemm_strided_batched(job::threads::ThreadPool& pool,
                                  int batchCount,
                                  int M, int N, int K,
                                  float alpha,
                                  const float *A, int strideA, int lda,
                                  const float *B, int strideB, int ldb,
                                  float beta,
                                  float *C, int strideC, int ldc)
{
    job::threads::parallel_for(pool, size_t{0}, size_t(batchCount), [&](size_t i) {
        const float *a_ptr = A + (i * strideA);
        const float *b_ptr = B + (i * strideB);
        float *c_ptr       = C + (i * strideC);
        sgemm_raw(M, N, K, alpha, a_ptr, lda, b_ptr, ldb, beta, c_ptr, ldc);
    });
}

// Serial SGEMM with Matrix objects
inline void sgemm(const Matrix &A, const Matrix &B, Matrix &C,
                  float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.cols() == B.rows() && "Inner dimensions must match (K)");
    assert(C.rows() == A.rows() && "Output rows must match A rows");
    assert(C.cols() == B.cols() && "Output cols must match B cols");

    sgemm_raw(A.rows(), B.cols(), A.cols(),
              alpha, A.data(), A.extent()[1], // lda = K
              B.data(), B.extent()[1],        // ldb = N
              beta, C.data(), C.extent()[1]); // ldc = N
}

// Parallel SGEMM with Matrix objects
inline void sgemm_parallel(job::threads::ThreadPool &pool,
                           const Matrix &A, const Matrix& B, Matrix& C,
                           float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.cols() == B.rows());
    assert(C.rows() == A.rows());
    assert(C.cols() == B.cols());
    sgemm_parallel_raw(pool,
                       A.rows(), B.cols(), A.cols(),
                       alpha, A.data(), A.extent()[1],
                       B.data(), B.extent()[1],
                       beta, C.data(), C.extent()[1]);
}


} // namespace job::ai::comp
