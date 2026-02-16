#include "ai_weights.h"
#include "job_stealing_ctx.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

#include <aligned_allocator.h>
#include <matrix.h>
#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>

#include <gemm.h>
using Catch::Approx;
using job::core::AlignedAllocator;
using namespace job::threads;
using namespace job::ai::cords;
using namespace job::ai::comp;
template <typename Vec>
static void fillMatrix(Vec &m, float scale = 0.01f)
{
    for (std::size_t i = 0; i < m.size(); ++i)
        m[i] = scale * static_cast<float>((static_cast<int>(i % 31) - 15));
}


template <typename Vec>
static void compareMats(const Vec& C_ref,
                        const Vec& C_test,
                        int M, int N,
                        float tol = float(1e-4))
{
    REQUIRE(C_ref.size() == C_test.size());

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            auto idx = static_cast<std::size_t>(i * N + j);
            // Catch2 Approx check
            if (C_test[idx] != Approx(static_cast<double>(C_ref[idx])).margin(static_cast<double>(tol))) {
                FAIL("Mismatch at (" << i << ", " << j << ") Ref: " << C_ref[idx] << " Test: " << C_test[idx]);
            }
        }
    }
}

TEST_CASE("sgemm: correctness vs naive reference on small shapes", "[ai][sgemm][correctness]")
{
    struct Shape { int M, N, K; };
    std::vector<Shape> shapes = {
        {1, 1, 1},
        {2, 3, 4},
        {5, 7, 3},
        {8, 8, 8},    // Aligned
        {9, 10, 11},  // Misaligned
        {16, 16, 16},
        {33, 33, 33}  // Larger than micro-kernel
    };

    for (const auto& s : shapes) {
        const int M = s.M;
        const int N = s.N;
        const int K = s.K;

        const int lda = K; // Row major
        const int ldb = N;
        const int ldc = N;

        // Use standard vector for small correctness tests is fine
        std::vector<float> A(M * K);
        std::vector<float> B(K * N);

        fillMatrix(A, 0.1f);
        fillMatrix(B, 0.05f);

        std::vector<float> C_ref(M * N);
        std::vector<float> C_test(M * N);

        SECTION("Shape " + std::to_string(M) + "x" + std::to_string(N) + "x" + std::to_string(K)) {
            std::fill(C_ref.begin(), C_ref.end(), 0.0f);
            std::fill(C_test.begin(), C_test.end(), 0.0f);

            sgemmNaive(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_ref.data(), ldc);
            sgemm(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_test.data(), ldc);

            compareMats(C_ref, C_test, M, N);

            sgemmNaive(M, N, K, 0.5f, A.data(), lda, B.data(), ldb, 1.0f, C_ref.data(), ldc);
            sgemm(M, N, K, 0.5f, A.data(), lda, B.data(), ldb, 1.0f, C_test.data(), ldc);

            compareMats(C_ref, C_test, M, N);
        }
    }
}




TEST_CASE("sgemm: Matrix Object API Correctness", "[ai][sgemm][matrix]")
{
    // Small shape for correctness
    constexpr int M = 128;
    constexpr int K = 256;
    constexpr int N = 128;

    AiWeights A_vec(M * K);
    AiWeights B_vec(K * N);
    AiWeights C_raw(M * N);
    AiWeights C_obj(M * N);

    fillMatrix(A_vec, 0.1f);
    fillMatrix(B_vec, 0.05f);
    std::fill(C_raw.begin(), C_raw.end(), 0.0f);
    std::fill(C_obj.begin(), C_obj.end(), 0.0f);

    sgemm(M, N, K, 1.0f, A_vec.data(), K, B_vec.data(), N, 0.0f, C_raw.data(), N);

    Matrix matA(A_vec.data(), M, K);
    Matrix matB(B_vec.data(), K, N);
    Matrix matC(C_obj.data(), M, N);

    sgemmMatrix(matA, matB, matC, 1.0f, 0.0f);
    compareMats(C_raw, C_obj, M, N);
    std::fill(C_obj.begin(), C_obj.end(), 0.0f);

    auto sched = std::make_shared<job::threads::FifoScheduler>();
    auto pool = job::threads::ThreadPool::create(sched, 4);

    sgemmParallelMatrix(*pool, matA, matB, matC, 1.0f, 0.0f);

    compareMats(C_raw, C_obj, M, N);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("sgemm: Parallel Scaling (Single vs Multi-Thread)", "[ai][sgemm][bench][scaling]")
{
    constexpr int M = 512;
    constexpr int K = 1024;
    constexpr int N = 1024;
    const int lda = K;
    const int ldb = N;
    const int ldc = N;

    AiWeights A(M * K, 0.01f);
    AiWeights B(K * N, 0.01f);
    AiWeights C(M * N, 0.0f);

    // Setup ThreadPool (Use 8 cores)
    auto sched = std::make_shared<job::threads::FifoScheduler>();
    auto pool = job::threads::ThreadPool::create(sched, 8);

    BENCHMARK("Serial AVX + Tiling SGEMM (m=512 K=1024 N=1024)") {
        sgemm(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C.data(), ldc);
        return C[0];
    };

    BENCHMARK("Parallel(8) + Tiling + AVX SGEMM (m=512 K=1024 N=1024)") {
        sgemmParallel(*pool, M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C.data(), ldc);
        return C[0];
    };
}


TEST_CASE("SGEMM Showdown: Naive vs Optimized", "[ai][sgemm][bench][vs]")
{
    // Use smaller dimensions so Naive doesn't take all day.
    // 16 * 1024 * 1024 is substantial enough to show cache effects,
    // but small enough for Naive to finish in ~50ms per run.
    constexpr int M = 16;
    constexpr int K = 1024;
    constexpr int N = 1024;

    const int lda = K;
    const int ldb = N;
    const int ldc = N;

    AiWeights A(M * K, 1.0f);
    AiWeights B(K * N, 0.5f);
    AiWeights C_naive(M * N, 0.0f);
    AiWeights C_opt(M * N, 0.0f);

    // Run Naive (The Control Group)
    BENCHMARK("Naive Implementation (Scalar loops)") {
        sgemmNaive(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_naive.data(), ldc);
        return C_naive[0];
    };

    // Run Optimized (The Contender)
    BENCHMARK("Optimized Implementation (AVX2 + Tiling)") {
        sgemm(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_opt.data(), ldc);
        return C_opt[0];
    };
}

TEST_CASE("sgemm: Matrix Object vs Raw Overhead", "[ai][sgemm][bench][matrix]")
{

    JobStealerCtx ctx(8);

    // Use a medium size to check if the abstraction adds latency
    constexpr int M = 1024;
    constexpr int K = 1024;
    constexpr int N = 1024;

    AiWeights A_vec(M * K, 0.01f);
    AiWeights B_vec(K * N, 0.01f);
    AiWeights C_vec(M * N, 0.0f);

    Matrix matA(A_vec.data(), M, K);
    Matrix matB(B_vec.data(), K, N);
    Matrix matC(C_vec.data(), M, N);

    BENCHMARK("Raw Pointer SGEMM (1024^3)") {
        sgemm(M, N, K, 1.0f, A_vec.data(), K, B_vec.data(), N, 0.0f, C_vec.data(), N);
        return C_vec[0];
    };

    BENCHMARK("Matrix Object SGEMM (1024^3)") {
        sgemmMatrix(matA, matB, matC, 1.0f, 0.0f);
        return C_vec[0];
    };

    BENCHMARK("Matrix Object Parallel(8) SGEMM (1024^3)") {
        sgemmParallelMatrix(*ctx.pool, matA, matB, matC, 1.0f, 0.0f);
        return C_vec[0];
    };
}
#endif
