#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <vector>
#include <cmath>
#include <random>


#include <job_stealing_ctx.h>
#include <flash_attention.h>

using namespace job::ai::comp;
using namespace job::threads;

void naiveAttention(int N, int d,
                    const float *Q, const float *K, const float *V, float *O,
                    float scale)
{
    std::vector<float> S(N * N);

    // Q * K^T
    for(int i=0; i<N; ++i) {
        for(int j=0; j<N; ++j) {
            float sum = 0.0f;
            for(int k=0; k<d; ++k) {
                sum += Q[i*d + k] * K[j*d + k];
            }
            S[i*N + j] = sum * scale;
        }
    }

    // "softmax"
    for(int i=0; i<N; ++i) {
        float max_val = -1e9;
        for(int j=0; j<N; ++j) max_val = std::max(max_val, S[i*N+j]);

        float sum = 0.0f;
        for(int j=0; j<N; ++j) {
            S[i*N+j] = std::exp(S[i*N+j] - max_val);
            sum += S[i*N+j];
        }
        for(int j=0; j<N; ++j) S[i*N+j] /= sum;
    }

    // S * V
    for(int i=0; i<N; ++i) {
        for(int k=0; k<d; ++k) {
            float sum = 0.0f;
            for(int j=0; j<N; ++j) {
                sum += S[i*N+j] * V[j*d+k];
            }
            O[i*d+k] = sum;
        }
    }
}

TEST_CASE("FlashAttention Correctness", "[ai][att][correctness]") {
    int N = 128;
    int d = 64;
    float scale = 1.0f / std::sqrt(static_cast<float>(d));

    std::vector<float> Q(N*d), K(N*d), V(N*d);
    std::vector<float> O_ref(N*d), O_flash(N*d);

    for(auto& x : Q)
        x = (rand() % 100) / 100.0f;

    for(auto& x : K)
        x = (rand() % 100) / 100.0f;

    for(auto& x : V)
        x = (rand() % 100) / 100.0f;

    // naive
    naiveAttention(N, d, Q.data(), K.data(), V.data(), O_ref.data(), scale);

    // flash


    JobStealerCtx ctx(8);
    flashAttentionForward(*ctx.pool, N, d, Q.data(), K.data(), V.data(), O_flash.data(), scale);

    for(int i=0; i<N*d; ++i)
        REQUIRE_THAT(O_flash[i], Catch::Matchers::WithinRel(O_ref[i], 0.001f));
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("FlashAttention Benchmark", "[ai][att][bench]") {
    int N = 2048; // "Realistic" Sequence Length
    int d = 64;   // "Standard" Head Dim
    float scale = 1.0f / std::sqrt(64.0f);

    std::vector<float> Q(N*d, 0.5f);
    std::vector<float> K(N*d, 0.5f);
    std::vector<float> V(N*d, 0.5f);
    std::vector<float> O(N*d);

    JobStealerCtx ctx(8);

    BENCHMARK("FlashAttention Forward (N=2048)") {
        flashAttentionForward(*ctx.pool, N, d, Q.data(), K.data(), V.data(), O.data(), scale);
        return O[0];
    };
}


TEST_CASE("The Showdown FlashAttention Benchmark", "[ai][att][bench]") {
    int N = 512; // keep the numbers down for naive
    int d = 64;   // Head Dim
    float scale = 1.0f / std::sqrt(64.0f);

    std::vector<float> Q(N*d, 0.5f);
    std::vector<float> K(N*d, 0.5f);
    std::vector<float> V(N*d, 0.5f);
    std::vector<float> O(N*d);

    JobStealerCtx ctx(8);

    BENCHMARK("FlashAttention Forward (N=512 dim=64)") {
        flashAttentionForward(*ctx.pool, N, d, Q.data(), K.data(), V.data(), O.data(), scale);
        return O[0];
    };

    BENCHMARK("Naive Attention (N=512 dim=64)") {
        naiveAttention(N, d, Q.data(), K.data(), V.data(), O.data(), scale);
        return O[0];
    };
}
#endif
