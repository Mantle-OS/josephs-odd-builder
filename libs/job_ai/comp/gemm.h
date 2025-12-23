#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>

#include <job_thread_pool.h>
#include <job_parallel_for.h>

#include <simd_provider.h>

#include "matrix.h"

namespace job::ai::comp {
using namespace job::ai::cords;
using namespace job::simd;

constexpr int kMicroM = 8;
constexpr int kMicroN = 8;

//  micro kernel (8x8)
// Computes 8x8 block: C = alpha * (A * B) + C
__attribute__((always_inline))
inline void microKernel8x8(int K, float alpha,
               const float* __restrict__ A, int lda,
               const float* __restrict__ B, int ldb,
               float* __restrict__ C, int ldc)
{
    f32 c0 = SIMD::zero();
    f32 c1 = SIMD::zero();
    f32 c2 = SIMD::zero();
    f32 c3 = SIMD::zero();
    f32 c4 = SIMD::zero();
    f32 c5 = SIMD::zero();
    f32 c6 = SIMD::zero();
    f32 c7 = SIMD::zero();

    for (int p = 0; p < K; ++p) {
        f32 b_row = SIMD::pull(B + p * ldb);
        c0 = SIMD::mul_plus(SIMD::set1(A[0 * lda + p]), b_row, c0);
        c1 = SIMD::mul_plus(SIMD::set1(A[1 * lda + p]), b_row, c1);
        c2 = SIMD::mul_plus(SIMD::set1(A[2 * lda + p]), b_row, c2);
        c3 = SIMD::mul_plus(SIMD::set1(A[3 * lda + p]), b_row, c3);
        c4 = SIMD::mul_plus(SIMD::set1(A[4 * lda + p]), b_row, c4);
        c5 = SIMD::mul_plus(SIMD::set1(A[5 * lda + p]), b_row, c5);
        c6 = SIMD::mul_plus(SIMD::set1(A[6 * lda + p]), b_row, c6);
        c7 = SIMD::mul_plus(SIMD::set1(A[7 * lda + p]), b_row, c7);
    }

    f32 alpha_vec = SIMD::set1(alpha);

#define UPDATE_REG(reg, row_idx) \
    reg = SIMD::mul_plus(reg, alpha_vec, SIMD::pull(C + (row_idx) * ldc)); \
        SIMD::mov(C + (row_idx) * ldc, reg)


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

inline void maskedStore(float* ptr, int valid_n, f32 val) {
    if (valid_n >= 8) {
        SIMD::mov(ptr, val); // Full store
    } else {
        // Create a mask for valid lanes
        // Note: Faster to use a jump table or pre-computed masks,
        // but this illustrates the logic using your existing provider.
        alignas(32) float temp[8];
        SIMD::mov(temp, val);
        for (int i = 0; i < valid_n; ++i) {
            ptr[i] = temp[i];
        }
    }
}

inline void computeTile(int M, int N, int K, float alpha,
                         const float *A, int lda,
                         const float *B, int ldb,
                         float *C, int ldc,
                         int start_m, int start_n, int block_size)
{
    int M_curr = std::min(block_size, M - start_m);
    int N_curr = std::min(block_size, N - start_n);

    for (int mi = 0; mi < M_curr; mi += kMicroM) {
        for (int ni = 0; ni < N_curr; ni += kMicroN) {

            // perfect 8x8 Block (Hot Path)
            if (mi + kMicroM <= M_curr && ni + kMicroN <= N_curr) {
                microKernel8x8(K, alpha,
                               &A[(start_m + mi) * lda], lda,
                               &B[(start_n + ni)], ldb,
                               &C[(start_m + mi) * ldc + (start_n + ni)], ldc);
            }  else {
                int m_limit = std::min(kMicroM, M_curr - mi); // How many rows are valid?
                int n_limit = std::min(kMicroN, N_curr - ni); // How many cols are valid?

                [[maybe_unused]]f32 c[8];
                for(int i=0; i<8; ++i)
                    c[i] = SIMD::zero();

                // Registers
                f32 acc[8];
                for(int i=0; i<8; ++i) acc[i] = SIMD::zero();

                for (int p = 0; p < K; ++p) {
                    f32 b_row;
                    if (n_limit >= 8) {
                        b_row = SIMD::pull(B + p * ldb + (start_n + ni));
                    } else {
                        alignas(32) float tmp_b[8] = {0};
                        for(int k=0; k<n_limit; ++k)
                            tmp_b[k] = B[p * ldb + (start_n + ni) + k];

                        b_row = SIMD::pull(tmp_b);
                    }

                    // Only iterate valid rows
                    for (int i = 0; i < m_limit; ++i) {
                        f32 a_val = SIMD::set1(A[(start_m + mi + i) * lda + p]);
                        acc[i] = SIMD::mul_plus(a_val, b_row, acc[i]);
                    }
                }

                // Update C (Masked Store)
                f32 v_alpha = SIMD::set1(alpha);
                for (int i = 0; i < m_limit; ++i) {
                    float* c_ptr = &C[(start_m + mi + i) * ldc + (start_n + ni)];

                    // Load existing C (Masked)
                    f32 c_curr;
                    if (n_limit >= 8) {
                        c_curr = SIMD::pull(c_ptr);
                    } else {
                        alignas(32) float tmp_c[8] = {0};
                        for(int k=0; k<n_limit; ++k) tmp_c[k] = c_ptr[k];
                        c_curr = SIMD::pull(tmp_c);
                    }

                    // acc = alpha * acc + C
                    acc[i] = SIMD::mul_plus(acc[i], v_alpha, c_curr);

                    // Store back
                    maskedStore(c_ptr, n_limit, acc[i]);
                }
            }
        }
    }
}


// Pure scalar tile computation (No SIMD, No MicroKernels, Just Math)
inline void computeTileNaive(int M, int N, int K, float alpha,
                               const float *A, int lda,
                               const float *B, int ldb,
                               float *C, int ldc,
                               int start_m, int start_n, int block_size)
{
    int M_curr = std::min(block_size, M - start_m);
    int N_curr = std::min(block_size, N - start_n);

    // Simple triple loop for the block
    for (int i = 0; i < M_curr; ++i) {
        for (int j = 0; j < N_curr; ++j) {
            float sum = 0.0f;
            // K-dimension dot product
            for (int p = 0; p < K; ++p) {
                sum += A[(start_m + i) * lda + p] * B[p * ldb + (start_n + j)];
            }
            // Accumulate: C = alpha*sum + C (Beta handled in outer loop)
            C[(start_m + i) * ldc + (start_n + j)] += alpha * sum;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

// Raw pointer sgemm with tile
inline void sgemm(int M, int N, int K,
                  float alpha, const float *A, int lda,
                  const float *B, int ldb,
                  float beta, float *C, int ldc)
{
    // beta scaling
    if (beta == 0.0f) {
        for(int i=0; i<M; ++i)
            std::memset(C + i * ldc, 0, N * sizeof(float));
    } else if (beta != 1.0f) {
        for(int i=0; i<M; ++i)
            for(int j=0; j<N; ++j)
                C[i * ldc + j] *= beta;
    }

    // Tiling
    for (int i = 0; i < M; i += JOB_BLOCK_SIZE)
        for (int j = 0; j < N; j += JOB_BLOCK_SIZE)
            computeTile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, JOB_BLOCK_SIZE);
}


// Naive sgemm (useful for benchmarks) (FIXME computeTileNaive)
inline void sgemmNaive(int M, int N, int K,
                       float alpha,
                       const float *A, int lda,
                       const float *B, int ldb,
                       float beta,
                       float *C, int ldc)
{
    if (beta == 0.0f) {
        for(int i=0; i<M; ++i)
            std::memset(C + i * ldc, 0, N * sizeof(float));
    } else if (beta != 1.0f) {
        for(int i=0; i<M; ++i)
            for(int j=0; j<N; ++j)
                C[i * ldc + j] *= beta;
    }

    for (int i = 0; i < M; i += JOB_BLOCK_SIZE) {
        for (int j = 0; j < N; j += JOB_BLOCK_SIZE) {
            computeTileNaive(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, JOB_BLOCK_SIZE);
        }
    }
}


// low-level raw pointer parallel sgemm
inline void sgemmParallel(job::threads::ThreadPool &pool,
                               int M, int N, int K,
                               float alpha, const float *A, int lda,
                               const float *B, int ldb,
                               float beta, float *C, int ldc)
{
    // beta scaling (row parallel)
    job::threads::parallel_for(pool, size_t{0}, size_t(M), [&](size_t i) {
        if (beta == 0.0f) {
            std::memset(C + i * ldc, 0, N * sizeof(float));
        } else if (beta != 1.0f) {
            for(int j=0; j<N; ++j)
                C[i * ldc + j] *= beta;
        }
    });

    // 2d block parallelization
    int m_chunks = (M + JOB_BLOCK_SIZE - 1) / JOB_BLOCK_SIZE;
    int n_chunks = (N + JOB_BLOCK_SIZE - 1) / JOB_BLOCK_SIZE;
    size_t total_tiles = m_chunks * n_chunks;

    job::threads::parallel_for(pool, size_t{0}, total_tiles, [&](size_t tile_idx) {
        int chunk_m = tile_idx / n_chunks;
        int chunk_n = tile_idx % n_chunks;
        int i = chunk_m * JOB_BLOCK_SIZE;
        int j = chunk_n * JOB_BLOCK_SIZE;

        computeTile(M, N, K, alpha, A, lda, B, ldb, C, ldc, i, j, JOB_BLOCK_SIZE);
    }, 0, 1); // grain size 1 to prevent timeouts
}


///////////////////////////////////////////////////////////////////////////////////////////////



// FIXME SWAP OUT THE job::threads::parallel_for for serial for loop
inline void sgemmStridedBatched(int batchCount,
                                int M, int N, int K,
                                float alpha,
                                const float *A, int strideA, int lda,
                                const float *B, int strideB, int ldb,
                                float beta,
                                float *C, int strideC, int ldc)
{
    for (int i = 0; i < batchCount; ++i) {
        const float *a_ptr = A + (i * strideA);
        const float *b_ptr = B + (i * strideB);
        float *c_ptr       = C + (i * strideC);
        sgemm(M, N, K, alpha, a_ptr, lda, b_ptr, ldb, beta, c_ptr, ldc);
    }
}


// FIXME SWAP OUT THE job::threads::parallel_for for serial for loop
inline void sgemmNaiveStridedBatched( int batchCount,
                                int M, int N, int K,
                                float alpha,
                                const float *A, int strideA, int lda,
                                const float *B, int strideB, int ldb,
                                float beta,
                                float *C, int strideC, int ldc)
{
    for (int i = 0; i < batchCount; ++i) {
        const float *a_ptr = A + (i * strideA);
        const float *b_ptr = B + (i * strideB);
        float *c_ptr       = C + (i * strideC);
        sgemmNaive(M, N, K, alpha, a_ptr, lda, b_ptr, ldb, beta, c_ptr, ldc);
    }
}

inline void sgemmParallelStridedBatched(job::threads::ThreadPool &pool,
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
        sgemm(M, N, K, alpha, a_ptr, lda, b_ptr, ldb, beta, c_ptr, ldc);
    });
}



///////////////////////////////////////////////////////////////////////////////////////////////

// serial sgemm with matrix objects
inline void sgemmMatrix(const Matrix &A, const Matrix &B, Matrix &C,
                  float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.cols() == B.rows() && "Inner dimensions must match (K)");
    assert(C.rows() == A.rows() && "Output rows must match A rows");
    assert(C.cols() == B.cols() && "Output cols must match B cols");

    sgemm(A.rows(), B.cols(), A.cols(),
              alpha, A.data(), A.extent()[1], // lda = K
              B.data(), B.extent()[1],        // ldb = N
              beta, C.data(), C.extent()[1]); // ldc = N
}

// navie sgemm with matrix objects
inline void sgemmNavieMatrix(const Matrix &A, const Matrix &B, Matrix &C,
                        float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.cols() == B.rows() && "Inner dimensions must match (K)");
    assert(C.rows() == A.rows() && "Output rows must match A rows");
    assert(C.cols() == B.cols() && "Output cols must match B cols");

    sgemmNaive(A.rows(), B.cols(), A.cols(),
               alpha, A.data(), A.extent()[1], // lda = K
               B.data(), B.extent()[1],        // ldb = N
               beta, C.data(), C.extent()[1]); // ldc = N
}


// parallel sgemm with matrix objects
inline void sgemmParallelMatrix(job::threads::ThreadPool &pool,
                           const Matrix &A, const Matrix &B, Matrix &C,
                           float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.cols() == B.rows());
    assert(C.rows() == A.rows());
    assert(C.cols() == B.cols());
    sgemmParallel(pool,
                  A.rows(), B.cols(), A.cols(),
                  alpha, A.data(), A.extent()[1],
                  B.data(), B.extent()[1],
                  beta, C.data(), C.extent()[1]);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// serial sgemm with ViewR objects
inline void sgemmView(const ViewR &A, const ViewR &B, ViewR &C,
                      float alpha = 1.0f, float beta = 0.0f)
{
    // Safety Checks
    assert(A.rank() == 2 && B.rank() == 2 && C.rank() == 2 && "Input views must be 2D");
    assert(A.extent()[1] == B.extent()[0] && "Inner dimensions (K) must match");
    assert(C.extent()[0] == A.extent()[0] && "C rows must match A rows (M)");
    assert(C.extent()[1] == B.extent()[1] && "C cols must match B cols (N)");

    // Extract Dimensions
    int M = static_cast<int>(A.extent()[0]);
    int N = static_cast<int>(B.extent()[1]);
    int K = static_cast<int>(A.extent()[1]);

    // Call Raw Kernel
    // stride(0) corresponds to the 'width' (LDA/LDB/LDC) in this dense View implementation
    sgemm(M, N, K,
          alpha,
          A.data(), static_cast<int>(A.stride(0)),
          B.data(), static_cast<int>(B.stride(0)),
          beta,
          C.data(), static_cast<int>(C.stride(0)));
}

// naive sgemm with ViewR objects
inline void sgemmNavieView(const ViewR &A, const ViewR &B, ViewR &C,
                           float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.rank() == 2 && B.rank() == 2 && C.rank() == 2 && "Input views must be 2D");
    assert(A.extent()[1] == B.extent()[0] && "Inner dimensions (K) must match");
    assert(C.extent()[0] == A.extent()[0] && "C rows must match A rows (M)");
    assert(C.extent()[1] == B.extent()[1] && "C cols must match B cols (N)");

    int M = static_cast<int>(A.extent()[0]);
    int N = static_cast<int>(B.extent()[1]);
    int K = static_cast<int>(A.extent()[1]);

    sgemmNaive(M, N, K,
               alpha,
               A.data(), static_cast<int>(A.stride(0)),
               B.data(), static_cast<int>(B.stride(0)),
               beta,
               C.data(), static_cast<int>(C.stride(0)));
}


// parallel sgemm with ViewR objects
inline void sgemmParallelView(job::threads::ThreadPool &pool,
                              const ViewR &A, const ViewR &B, ViewR &C,
                              float alpha = 1.0f, float beta = 0.0f)
{
    assert(A.rank() == 2 && B.rank() == 2 && C.rank() == 2 && "Input views must be 2D");
    assert(A.extent()[1] == B.extent()[0] && "Inner dimensions (K) must match");
    assert(C.extent()[0] == A.extent()[0] && "C rows must match A rows (M)");
    assert(C.extent()[1] == B.extent()[1] && "C cols must match B cols (N)");

    int M = static_cast<int>(A.extent()[0]);
    int N = static_cast<int>(B.extent()[1]);
    int K = static_cast<int>(A.extent()[1]);

    sgemmParallel(pool,
                  M, N, K,
                  alpha,
                  A.data(), static_cast<int>(A.stride(0)),
                  B.data(), static_cast<int>(B.stride(0)),
                  beta,
                  C.data(), static_cast<int>(C.stride(0)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





} // namespace job::ai::comp
