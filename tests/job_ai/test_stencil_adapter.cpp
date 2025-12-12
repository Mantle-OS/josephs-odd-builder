#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <algorithm>
#include <random>

#include <ctx/job_stealing_ctx.h>
#include <view.h>
#include <stencil_adapter.h>

using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;

TEST_CASE("Stencil Adapter: Heat Diffusion (3x3 Grid)", "[ai][stencil][usage]")
{
    // 4 workers to handle the parallel dimension/batch processing
    JobStealerCtx ctx(4);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.1f;
    cfg.boundary = BoundaryMode::FixedZero; // Edges are cold vacuum

    StencilAdapter adapter(cfg);

    // Geometry: 1 Batch, Sequence 9 (3x3 grid), Dim 1 (Single channel heat map)
    int B = 1;
    int S = 9;
    int D = 1;

    std::vector<float> data(S * D, 0.0f);

    // Set Center (index 4) to Hot
    // Grid:
    // 0 0 0
    // 0 H 0
    // 0 0 0
    data[4] = 100.0f;

    ViewR view(data.data(), ViewR::Extent{1, (uint32_t)S});
    // In-place modification is allowed by IAdapter contract usually,
    // but here we act like 'output' is the write target.
    // For this test, let's write back to the same buffer to mimic a residual update or state update.
    ViewR out(data.data(), ViewR::Extent{1, (uint32_t)S});

    AttentionShape shape{ (uint32_t)B, (uint32_t)S, (uint32_t)D, 1 };
    AdapterCtx aCtx; // Defaults

    adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);

    // Verify Physics
    // Laplacian Rule: New = Old + Rate * (Sum_Neighbors - 4*Old)
    // Center Neighbors are all 0. Sum = 0.
    // Center New = 100 + 0.1 * (0 - 400) = 100 - 40 = 60.
    CHECK(data[4] == Catch::Approx(60.0f).epsilon(0.01));

    // Neighbor (e.g., index 3, to the left)
    // Old = 0. Neighbors = 100 (center) + 0 + 0 + 0 = 100.
    // Neighbor New = 0 + 0.1 * (100 - 0) = 10.
    CHECK(data[3] == Catch::Approx(10.0f).epsilon(0.01)); // Left
    CHECK(data[5] == Catch::Approx(10.0f).epsilon(0.01)); // Right
    CHECK(data[1] == Catch::Approx(10.0f).epsilon(0.01)); // Top
    CHECK(data[7] == Catch::Approx(10.0f).epsilon(0.01)); // Bottom

    // Corners should still be 0 (Manhattan distance > 1)
    CHECK(data[0] == 0.0f);
}

// BLOCK 2: Edge Cases
TEST_CASE("Stencil Adapter: Dimension Isolation", "[ai][stencil][edge]")
{
    // Ensure that physics in Dimension 0 do not bleed into Dimension 1.
    // This verifies that the "transpose" or striding logic is correct.
    JobStealerCtx ctx(2);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.5f;
    cfg.boundary = BoundaryMode::Wrap;

    StencilAdapter adapter(cfg);

    int S = 4; // 2x2 Grid
    int D = 2; // Two distinct chemical channels

    std::vector<float> data(S * D, 0.0f);

    // Dim 0: All 100s (Uniform heat, should not change)
    // Dim 1: All 0s (Absolute zero, should not change)
    for(int i=0; i<S; ++i) {
        data[i * D + 0] = 100.0f;
        data[i * D + 1] = 0.0f;
    }

    ViewR view(data.data(), ViewR::Extent{1, (uint32_t)S});
    ViewR out(data.data(), ViewR::Extent{1, (uint32_t)S});
    AttentionShape shape{ 1, (uint32_t)S, (uint32_t)D, 1 };
    AdapterCtx aCtx;

    adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);

    for(int i=0; i<S; ++i) {
        // Dim 0 should remain 100 (Sum neighbors = 400, 4*Center = 400, Delta = 0)
        CHECK(data[i * D + 0] == Catch::Approx(100.0f));
        // Dim 1 should remain 0 (No heat from Dim 0 leaked in)
        CHECK(data[i * D + 1] == 0.0f);
    }
}

TEST_CASE("Stencil Adapter: Zero Rate (Identity)", "[ai][stencil][edge]")
{
    JobStealerCtx ctx(1);
    StencilConfig cfg;
    cfg.diffusionRate = 0.0f; // Nothing moves
    StencilAdapter adapter(cfg);

    int S = 9;
    int D = 1;
    std::vector<float> data(S * D);
    for(int i=0; i<S; ++i) data[i] = (float)i;

    ViewR view(data.data(), ViewR::Extent{1, (uint32_t)S});
    ViewR out(data.data(), ViewR::Extent{1, (uint32_t)S});
    AttentionShape shape{ 1, (uint32_t)S, (uint32_t)D, 1 };
    AdapterCtx aCtx;

    adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);

    // Should be unchanged
    for(int i=0; i<S; ++i) {
        CHECK(data[i] == (float)i);
    }
}

// BLOCK 3: Benchmarks
#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Stencil Adapter: Throughput", "[ai][stencil][bench]")
{
    // Stencil is memory bound. We want to saturate the bandwidth.
    JobStealerCtx ctx(8);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.1f;
    cfg.boundary = BoundaryMode::Wrap;

    StencilAdapter adapter(cfg);
    AdapterCtx aCtx;

    auto run_bench = [&](int seq_len, int dims) {
        int B = 4; // Moderate batch size
        size_t total = (size_t)B * seq_len * dims;
        std::vector<float> data(total, 1.0f);

        // Randomize slightly to avoid compiler optimizing uniform math
        data[0] = 5.0f;
        data[total-1] = 10.0f;

        ViewR view(data.data(), ViewR::Extent{(uint32_t)B, (uint32_t)seq_len});
        ViewR out(data.data(), ViewR::Extent{(uint32_t)B, (uint32_t)seq_len});
        AttentionShape shape{ (uint32_t)B, (uint32_t)seq_len, (uint32_t)dims, 1 };

        adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);
    };

    // Small Context (ViT patch size equivalent? 16x16 = 256)
    BENCHMARK("Stencil S=256 D=64 (Small)") {
        run_bench(256, 64);
    };

    // Medium Context (32x32 = 1024)
    BENCHMARK("Stencil S=1024 D=128 (Medium)") {
        run_bench(1024, 128);
    };

    // Large Context (64x64 = 4096)
    // This stresses the Transpose logic heavily
    BENCHMARK("Stencil S=4096 D=256 (Large)") {
        run_bench(4096, 256);
    };
}
#endif
