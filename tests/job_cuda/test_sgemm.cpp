#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <vector>


#include <job_logger.h>
#include <numeric>
#include <cmath>

// Assuming this is the header for your cu file
namespace job::cuda {
void sgemm(cublasHandle_t handle, int M, int N, int K,
           float alpha, const float* A, const float* B,
           float beta, float* C);
}

// RAII Wrapper for cuBLAS Handle to keep tests clean
struct CublasHandle {
    cublasHandle_t handle;
    CublasHandle() { cublasCreate(&handle); }
    ~CublasHandle() { cublasDestroy(handle); }
    operator cublasHandle_t() { return handle; }
};

// Helper for GPU memory
struct GpuBuffer {
    float* ptr = nullptr;
    size_t size;
    GpuBuffer(size_t n) : size(n * sizeof(float)) {
        cudaMalloc(&ptr, size);
    }
    ~GpuBuffer() { cudaFree(ptr); }
    void upload(const std::vector<float>& host) {
        cudaMemcpy(ptr, host.data(), size, cudaMemcpyHostToDevice);
    }
    void download(std::vector<float>& host) {
        cudaMemcpy(host.data(), ptr, size, cudaMemcpyDeviceToHost);
    }
};

TEST_CASE("sgemm: correctness on small shapes", "[cuda][sgemm][correctness]")
{
    CublasHandle handle;

    // Matrix Multiplication: 2x3 * 3x2 = 2x2
    // A = [1, 2, 3]  B = [7,  8]   Result C = [ 58,  64]
    //     [4, 5, 6]      [9, 10]              [139, 154]
    //                    [11, 12]

    constexpr int M = 2;
    constexpr int K = 3;
    constexpr int N = 2;

    std::vector<float> h_A = {1, 2, 3, 4, 5, 6};
    std::vector<float> h_B = {7, 8, 9, 10, 11, 12};
    std::vector<float> h_C(M * N, 0.0f);

    GpuBuffer d_A(M * K);
    GpuBuffer d_B(K * N);
    GpuBuffer d_C(M * N);

    d_A.upload(h_A);
    d_B.upload(h_B);

    job::cuda::sgemm(handle, M, N, K, 1.0f, d_A.ptr, d_B.ptr, 0.0f, d_C.ptr);

    d_C.download(h_C);

    SECTION("Validate values") {
        REQUIRE(h_C[0] == 58.0f);
        REQUIRE(h_C[1] == 64.0f);
        REQUIRE(h_C[2] == 139.0f);
        REQUIRE(h_C[3] == 154.0f);
    }
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("sgemm: benchmarks", "[cuda][sgemm][bench]")
{
    CublasHandle handle;
    constexpr int M = 512;
    constexpr int K = 1024;
    constexpr int N = 1024;

    std::vector<float> h_A(M * K, 0.5f);
    std::vector<float> h_B(K * N, 0.5f);
    std::vector<float> h_C(M * N);

    GpuBuffer d_A(M * K);
    GpuBuffer d_B(K * N);
    GpuBuffer d_C(M * N);

    d_A.upload(h_A);
    d_B.upload(h_B);

    BENCHMARK("cuBLAS SGEMM (Device Only)") {
        job::cuda::sgemm(handle, M, N, K, 1.0f, d_A.ptr, d_B.ptr, 0.0f, d_C.ptr);
        return cudaDeviceSynchronize();
    };

    BENCHMARK("cuBLAS SGEMM (Including Download)") {
        job::cuda::sgemm(handle, M, N, K, 1.0f, d_A.ptr, d_B.ptr, 0.0f, d_C.ptr);
        d_C.download(h_C);
        return h_C[0];
    };
}
#endif