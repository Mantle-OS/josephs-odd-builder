#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <stdexcept>

namespace job::cuda {

/**
 * Performs C = alpha * A * B + beta * C
 * Inputs A, B, and C are assumed to be in Row-Major order in GPU VRAM.
 */
void sgemm(cublasHandle_t handle, int M, int N, int K,
           float alpha, const float* A, const float* B,
           float beta, float* C)
{
    // Row-Major: A(M,K), B(K,N), C(M,N)
    // We treat them as Column-Major: A^T(K,M), B^T(N,K), C^T(N,M)
    // cuBLAS computes: C_out = alpha * op(Mat1) * op(Mat2) + beta * C_out
    // We want: C^T = alpha * B^T * A^T + beta * C^T

    // In Column-Major terms for cublasSgemm:
    // m = N (rows of C^T)
    // n = M (cols of C^T)
    // k = K (common dimension)
    // lda = N (Leading dimension of B^T is its number of rows, which is N)
    // ldb = K (Leading dimension of A^T is its number of rows, which is K)
    // ldc = N (Leading dimension of C^T is its number of rows, which is N)

    cublasStatus_t status = cublasSgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        N, M, K,
        &alpha,
        B, N,  // Matrix "A" in cublas is our B
        A, K,  // Matrix "B" in cublas is our A
        &beta,
        C, N   // Matrix "C" in cublas is our C
        );

    if (status != CUBLAS_STATUS_SUCCESS) {
        throw std::runtime_error("cuBLAS SGEMM failed");
    }
}

}
