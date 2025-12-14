#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <random>
#include <algorithm>

#include <job_thread_pool.h>
#include <ctx/job_stealing_ctx.h>

#include <fmm_adapter.h>
#include <aligned_allocator.h>

using namespace job::ai;
using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;
using Catch::Approx;

struct FmmTestCtx {
    JobStealerCtx threadCtx;
    FmmAdapter adapter;

    FmmTestCtx(int threads = 4) : threadCtx(threads) {
        FmmConfig cfg;
        cfg.theta = 0.5f;
        cfg.gravity = 1.0f; // G=1
        // Map dims 0,1,2 to x,y,z
        cfg.dim_mapping[0] = 0;
        cfg.dim_mapping[1] = 1;
        cfg.dim_mapping[2] = 2;
        adapter = FmmAdapter(cfg);
    }
};


bool is_finite_safe(float x)
{
    uint32_t bits;
    std::memcpy(&bits, &x, sizeof(float));
    return (bits & 0x7F800000u) != 0x7F800000u;
}

// USAGE / EXAMPLES
TEST_CASE("FMM Adapter: Semantic Gravity (Attraction)", "[ai][fmm][usage]") {
    // Scenario: Two tokens in a vacuum.
    // Token A at (0,0,0), Mass 1.
    // Token B at (2,0,0), Mass 100.
    // Token A should feel a strong force towards +X.
    // Token B should feel a weak force towards -X.

    FmmTestCtx ctx;

    int B = 1;
    int S = 2;
    int D = 4;

    std::vector<float> inputData(B * S * D, 0.0f);
    std::vector<float> outputData(B * S * D, 0.0f);

    // Token A (0,0,0)
    // Token B (2,0,0)
    inputData[1 * D + 0] = 2.0f;

    ViewR input(inputData.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));
    ViewR output(outputData.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));

    AttentionShape shape{(uint32_t)B, (uint32_t)S, (uint32_t)D, 1};
    AdapterCtx aCtx;
    aCtx.embedDim = D;

    ctx.adapter.adapt(*ctx.threadCtx.pool, shape, input, input, input, output, aCtx);

    // Analyze Force on Token A
    float fx_a = outputData[0];
    float fy_a = outputData[1];

    // Analyze Force on Token B
    float fx_b = outputData[1 * D + 0];

    INFO("Force on A (should be positive): " << fx_a);
    INFO("Force on B (should be negative): " << fx_b);

    // Physics Checks
    CHECK(fx_a > 0.2f);                 // Should be ~0.25
    CHECK(fx_b < -0.2f);                // Should be ~-0.25
    CHECK(fy_a == Approx(0.0f));        // No Y force

    // Newton's 3rd Law: Forces should be equal and opposite
    CHECK(fx_a == Approx(-fx_b).margin(0.001f));
}

// EDGE CASES
TEST_CASE("FMM Adapter: The Singularity (Overlapping Tokens)", "[ai][fmm][edge]") {
    // Scenario: All tokens stacked exactly at (0,0,0).
    // Physics says Force -> Infinity.
    // Implementation should be robust (softening parameter or skip self).

    FmmTestCtx ctx;

    int B = 1;
    int S = 16;
    int D = 4;

    std::vector<float> inputData(B * S * D, 0.0f);
    std::vector<float> outputData(B * S * D, 0.0f);

    ViewR input(inputData.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));
    ViewR output(outputData.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));

    AttentionShape shape{(uint32_t)B, (uint32_t)S, (uint32_t)D, 1};
    AdapterCtx aCtx;

    ctx.adapter.adapt(*ctx.threadCtx.pool, shape, input, input, input, output, aCtx);

    CHECK(is_finite_safe(outputData[0]));
    CHECK(outputData[0] == Approx(0.0f).margin(0.1f));
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("FMM Adapter: Scaling Benchmark (O(N) Proof)", "[ai][fmm][bench]") {
    // N = 4096 (Standard Transformer Context)
    // N = 16384 (Long Context)

    JobStealerCtx ctx(8); // Steal so that the pool does not hate us.

    // Config: Allow somewhat large leaves to speed up tree build
    FmmConfig cfg;
    cfg.maxLeaf = 128;
    FmmAdapter adapter(cfg);

    const int D = 64;
    AdapterCtx aCtx;
    aCtx.embedDim = D;

    auto run_bench = [&](int seq_len) {
        int B = 1;
        int S = seq_len;

        std::vector<float> data(B * S * D);
        // Random uniform distribution in 100x100x100 box
        // To prevent clustering at 0
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
        for(auto &x : data)
            x = dist(gen);

        ViewR view(data.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));
        AttentionShape shape{(uint32_t)B, (uint32_t)S, (uint32_t)D, 1};

        adapter.adapt(*ctx.pool, shape, view, view, view, view, aCtx);
    };

    BENCHMARK("FMM N=1024") { run_bench(1024); };
    BENCHMARK("FMM N=4096") { run_bench(4096); };

    // If it was O(N^2), 4096 would be 16x slower than 1024.
    // With O(N), it should be ~4x slower.
    //Need to look at linear scaling.
}
#endif
