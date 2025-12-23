#include "job_stealing_ctx.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>

#include <real_type.h>
#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>

#include <aligned_allocator.h>
#include <mlp.h>

using Catch::Approx;
using namespace job::ai::comp;
using namespace job::threads;

void mlpNaive(int B, int d_in, int d_hid,
               const float *X, const float *W1, const float *W2,
               float *Out)
{
    std::vector<float> H(B * d_hid, 0.0f);

    // X * W1 -> H
    for(int i = 0; i < B; ++i) {
        for(int j = 0; j < d_hid; ++j) {
            float sum = 0.0f;
            for(int k=0; k<d_in; ++k)
                sum += X[i*d_in + k] * W1[k*d_hid + j];

            H[i*d_hid + j] = sum;
        }
    }

    // ReLU Activation
    for(auto &val : H)
        val = std::max(0.0f, val);

    // H * W2 -> Out
    for(int i = 0; i < B; ++i) {
        for(int j = 0; j < d_in; ++j) {
            float sum = 0.0f;

            for(int k=0; k<d_hid; ++k)
                sum += H[i*d_hid + k] * W2[k*d_in + j];

            Out[i*d_in + j] = sum;
        }
    }
}

// FIXME crypto has this but this is quick and dirty
template <typename Vec>
void randomize(Vec &v)
{
    static std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    for(auto& x : v)
        x = dist(gen);
}

TEST_CASE("MLP: Correctness vs Naive", "[ai][mlp][correctness]")
{
    const int B = 4;
    const int D = 32;
    const int H = 64;

    using Alloc = AlignedAllocator<float, 64>;
    std::vector<float, Alloc> X(B * D);
    std::vector<float, Alloc> W1(D * H);
    std::vector<float, Alloc> W2(H * D);

    std::vector<float, Alloc> HiddenBuf(B * H);
    std::vector<float, Alloc> Out_Opt(B * D);
    std::vector<float, Alloc> Out_Ref(B * D);

    randomize(X);
    randomize(W1);
    randomize(W2);

    std::fill(Out_Ref.begin(), Out_Ref.end(), 0.0f);
    mlpNaive(B, D, H, X.data(), W1.data(), W2.data(), Out_Ref.data());

    JobStealerCtx ctx(8);

    mlpParallelForward(*ctx.pool, B, D, H, X.data(), W1.data(), W2.data(),
                HiddenBuf.data(), Out_Opt.data(), ActivationType::ReLU);

    for(size_t i = 0; i < Out_Ref.size(); ++i)
        REQUIRE_THAT(Out_Opt[i], Catch::Matchers::WithinRel(Out_Ref[i], 0.001f));
}

#ifdef JOB_TEST_BENCHMARKS

// BERT/GPT Style .... I think)
TEST_CASE("MLP: Benchmark Standard (GPT-3 Medium Sized ReLU)", "[ai][mlp][bench]")
{
    const int B = 32;    // Batch Size (Tokens)
    const int D = 1024;  // d_model
    const int H = 4096;  // d_hidden (4x expansion)

    using Alloc = AlignedAllocator<float, 64>;
    std::vector<float, Alloc> X(B * D, 0.1f);
    std::vector<float, Alloc> W1(D * H, 0.01f);
    std::vector<float, Alloc> W2(H * D, 0.01f);
    std::vector<float, Alloc> Hidden(B * H);
    std::vector<float, Alloc> Out(B * D);

    JobStealerCtx ctx(8);

    // FLOP Calculation:
    // 1. GEMM 1: 2 * B * D * H
    // 2. GEMM 2: 2 * B * H * D
    // Total Ops per run ~= 4 * 32 * 1024 * 4096 ~= 536 Million Ops

    BENCHMARK("MLP Forward (ReLU)") {
        mlpParallelForward(*ctx.pool, B, D, H, X.data(), W1.data(), W2.data(),
                    Hidden.data(), Out.data(), ActivationType::ReLU);
        return Out[0];
    };
}


// BENCHMARK: Gated MLP (LLaMA / PaLM Style)
// TEST_CASE("MLP: Benchmark Gated (Swish)", "[ai][mlp][bench][gated]")
// {
//     const int B = 16;     // Tokens
//     const int D = 4096;   // Input
//     const int H = 12288;  // Hidden (3x expansion is common in Swish)

//     using Alloc = AlignedAllocator<float, 64>;
//     std::vector<float, Alloc> X(B * D, 0.1f);

//     // gated uses 3 matrices
//     std::vector<float, Alloc> W_gate(D * H, 0.01f);
//     std::vector<float, Alloc> W_val(D * H, 0.01f);
//     std::vector<float, Alloc> W_proj(H * D, 0.01f);

//     // Needs 2 hidden buffers
//     std::vector<float, Alloc> GateBuf(B * H);
//     std::vector<float, Alloc> ValBuf(B * H);
//     std::vector<float, Alloc> Out(B * D);

//     JobStealerCtx ctx(8);

//     // 3 GEMMs instead of 2.
//     // Ops ~= 6 * B * D * H
//     // ~= 6 * 16 * 4096 * 12288 ~= 4.8 Billion FLOPs per run

//     BENCHMARK("Gated MLP Forward (Swish)") {
//         mlpParallelForward(*ctx.pool,
//                            B,
//                            D,
//                            H,
//                            X.data(),
//                            W_gate.data(),
//                            W_val.data(),
//                            W_proj.data(),
//                            GateBuf.data(),
//                            ValBuf.data(),
//                            Out.data(),
//                            ActivationType::Swish);
//         return Out[0];
//     };
// }

TEST_CASE("MLP: The Show down", "[ai][mlp][bench][compare]")
{
    // Smallish realistic unit: 16 Token, Small Embedding
    const int B = 16;    // 16 Token
    const int D = 128;   // Small Embedding
    const int H = 512;   // 4x Expansion

    using Alloc = AlignedAllocator<float, 64>;
    std::vector<float, Alloc> X(B * D, 0.5f);
    std::vector<float, Alloc> W1(D * H, 0.1f);
    std::vector<float, Alloc> W2(H * D, 0.1f);
    std::vector<float, Alloc> Hidden(B * H);
    std::vector<float, Alloc> Out(B * D);


    JobStealerCtx ctx(8);

    BENCHMARK("Naive") {
        mlpNaive(B, D, H, X.data(), W1.data(), W2.data(), Out.data());
        return Out[0];
    };

    BENCHMARK("ReLU (The Speed Demon)") {
        mlpParallelForward(*ctx.pool, B, D, H, X.data(), W1.data(), W2.data(),
                    Hidden.data(), Out.data(), ActivationType::ReLU);
        return Out[0];
    };

    BENCHMARK("GELU (The Transformer Standard)") {
        mlpParallelForward(*ctx.pool, B, D, H, X.data(), W1.data(), W2.data(),
                    Hidden.data(), Out.data(), ActivationType::GELU);
        return Out[0];
    };

    BENCHMARK("Swish (The LLaMA Style)") {
        mlpParallelForward(*ctx.pool, B, D, H, X.data(), W1.data(), W2.data(),
                    Hidden.data(), Out.data(), ActivationType::Swish);
        return Out[0];
    };
}
#endif
