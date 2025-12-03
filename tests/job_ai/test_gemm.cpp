#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

#include <real_type.h>
#include <aligned_allocator.h>

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>

#include <gemm.h>
using Catch::Approx;
using job::ai::base::sgemm_parallel;
using job::ai::base::sgemm;
using job::ai::base::sgemm_strided_batched; // Check snake_case vs CamelCase in gemm.h
using job::core::real_t;

using job::threads::ThreadPool;
using job::threads::FifoScheduler;


static void sgemmNaive(int M, int N, int K,
                       real_t alpha,
                       const real_t* A, int lda,
                       const real_t* B, int ldb,
                       real_t beta,
                       real_t* C, int ldc)
{
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            real_t sum = real_t(0);
            for (int p = 0; p < K; ++p) {
                sum += A[i * lda + p] * B[p * ldb + j];
            }
            C[i * ldc + j] = alpha * sum + beta * C[i * ldc + j];
        }
    }
}

template <typename Vec>
static void fillMatrix(Vec &m, real_t scale = real_t(0.01))
{
    for (std::size_t i = 0; i < m.size(); ++i)
        m[i] = scale * static_cast<real_t>((static_cast<int>(i % 31) - 15));
}


template <typename Vec>
static void compareMats(const Vec& C_ref,
                        const Vec& C_test,
                        int M, int N,
                        real_t tol = real_t(1e-4))
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
        std::vector<real_t> A(M * K);
        std::vector<real_t> B(K * N);

        fillMatrix(A, real_t(0.1));
        fillMatrix(B, real_t(0.05));

        std::vector<real_t> C_ref(M * N);
        std::vector<real_t> C_test(M * N);

        SECTION("Shape " + std::to_string(M) + "x" + std::to_string(N) + "x" + std::to_string(K)) {
            std::fill(C_ref.begin(), C_ref.end(), real_t(0));
            std::fill(C_test.begin(), C_test.end(), real_t(0));

            sgemmNaive(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_ref.data(), ldc);
            sgemm(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C_test.data(), ldc);

            compareMats(C_ref, C_test, M, N);

            sgemmNaive(M, N, K, 0.5f, A.data(), lda, B.data(), ldb, 1.0f, C_ref.data(), ldc);
            sgemm(M, N, K, 0.5f, A.data(), lda, B.data(), ldb, 1.0f, C_test.data(), ldc);

            compareMats(C_ref, C_test, M, N);
        }
    }
}

TEST_CASE("sgemm: Parallel Scaling (Single vs Multi-Thread)", "[ai][sgemm][bench][scaling]")
{
    constexpr int M = 512;
    constexpr int K = 1024;
    constexpr int N = 1024;
    const int lda = K;
    const int ldb = N;
    const int ldc = N;

    // Use Aligned Allocator
    using AlignedVec = std::vector<real_t, job::ai::base::AlignedAllocator<real_t, 64>>;
    AlignedVec A(M * K, 0.01f);
    AlignedVec B(K * N, 0.01f);
    AlignedVec C(M * N, 0.0f);

    // Setup ThreadPool (Use 8 cores)
    auto sched = std::make_shared<job::threads::FifoScheduler>();
    auto pool = job::threads::ThreadPool::create(sched, 8);

    BENCHMARK("Serial AVX + Tiling SGEMM (m=512 K=1024 N=1024)") {
        sgemm(M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C.data(), ldc);
        return C[0];
    };

    BENCHMARK("Parallel(8) + Tiling + AVX SGEMM (m=512 K=1024 N=1024)") {
        sgemm_parallel(*pool, M, N, K, 1.0f, A.data(), lda, B.data(), ldb, 0.0f, C.data(), ldc);
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

    // Allocate
    using AlignedVec = std::vector<real_t, job::ai::base::AlignedAllocator<real_t, 64>>;
    AlignedVec A(M * K, 1.0f);
    AlignedVec B(K * N, 0.5f);
    AlignedVec C_naive(M * N, 0.0f);
    AlignedVec C_opt(M * N, 0.0f);

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
