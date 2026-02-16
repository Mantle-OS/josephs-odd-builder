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
using namespace job::science;

TEST_CASE("Stencil Adapter: Heat Diffusion (3x3 Grid)", "[ai][stencil][usage]")
{
    JobStealerCtx ctx(4);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.1f;
    cfg.boundary = BoundaryMode::FixedZero; // Edges are cold vacuum

    StencilAdapter adapter(cfg);

    // Geometry: 1 Batch, Sequence 9 (3x3 grid), Dim 1 (Single channel heat map)
    const uint32_t B = 1;
    const uint32_t S = 9;
    const uint32_t D = 1;

    std::vector<float> data(S * D, 0.0f);

    // Center hot
    data[4 * D + 0] = 100.0f;

    ViewR view(data.data(), makeBSD(B, S, D));
    ViewR out (data.data(), makeBSD(B, S, D));

    AttentionShape shape{B, S, D, 1};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);

    // Index helper: (s, d) in [B=1] layout
    auto at = [&](int s, int d = 0) -> float& {
        return data[s * D + d];
    };

    // Center
    CHECK(at(4) == Catch::Approx(60.0f).epsilon(0.01));

    // 4-neighbors
    CHECK(at(3) == Catch::Approx(10.0f).epsilon(0.01)); // Left
    CHECK(at(5) == Catch::Approx(10.0f).epsilon(0.01)); // Right
    CHECK(at(1) == Catch::Approx(10.0f).epsilon(0.01)); // Top
    CHECK(at(7) == Catch::Approx(10.0f).epsilon(0.01)); // Bottom

    // Corners unchanged
    CHECK(at(0) == 0.0f);
}

TEST_CASE("Stencil Adapter: Dimension Isolation", "[ai][stencil][edge]")
{
    JobStealerCtx ctx(2);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.5f;
    cfg.boundary = BoundaryMode::Wrap;

    StencilAdapter adapter(cfg);

    const uint32_t B = 1;
    const uint32_t S = 4; // 2x2 grid
    const uint32_t D = 2; // two channels

    std::vector<float> data(S * D, 0.0f);

    for (uint32_t i = 0; i < S; ++i) {
        data[i * D + 0] = 100.0f; // channel 0
        data[i * D + 1] = 0.0f;   // channel 1
    }

    ViewR view(data.data(), makeBSD(B, S, D));
    ViewR out (data.data(), makeBSD(B, S, D));

    AttentionShape shape{B, S, D, 1};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);

    for (uint32_t i = 0; i < S; ++i) {
        CHECK(data[i * D + 0] == Catch::Approx(100.0f)); // uniform field -> no change
        CHECK(data[i * D + 1] == 0.0f);                  // no cross-channel leak
    }
}

TEST_CASE("Stencil Adapter: Zero Rate (Identity)", "[ai][stencil][edge]")
{
    JobStealerCtx ctx(1);

    StencilConfig cfg;
    cfg.diffusionRate = 0.0f; // Nothing moves
    cfg.boundary = BoundaryMode::Wrap; // whatever, rate=0 kills it anyway
    StencilAdapter adapter(cfg);

    const uint32_t B = 1;
    const uint32_t S = 9;
    const uint32_t D = 1;

    std::vector<float> data(S * D);
    for (uint32_t i = 0; i < S; ++i)
        data[i * D + 0] = static_cast<float>(i);

    ViewR view(data.data(), makeBSD(B, S, D));
    ViewR out (data.data(), makeBSD(B, S, D));

    AttentionShape shape{B, S, D, 1};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);

    for (uint32_t i = 0; i < S; ++i)
        CHECK(data[i * D + 0] == static_cast<float>(i));
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Stencil Adapter: Throughput", "[ai][stencil][bench]")
{
    JobStealerCtx ctx(8);

    StencilConfig cfg;
    cfg.steps = 1;
    cfg.diffusionRate = 0.1f;
    cfg.boundary = BoundaryMode::Wrap;

    StencilAdapter adapter(cfg);
    AdapterCtx aCtx;

    auto run_bench = [&](int seq_len, int dims) {
        const uint32_t B = 4;
        const uint32_t S = static_cast<uint32_t>(seq_len);
        const uint32_t D = static_cast<uint32_t>(dims);

        const std::size_t total = static_cast<std::size_t>(B) * S * D;
        std::vector<float> data(total, 1.0f);

        // Break uniformity a bit
        data[0] = 5.0f;
        data[total - 1] = 10.0f;

        ViewR view(data.data(), makeBSD(B, S, D));
        ViewR out (data.data(), makeBSD(B, S, D));

        AttentionShape shape{B, S, D, 1};

        adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);
    };

    BENCHMARK("Stencil S=256 D=64 (Small)") {
        run_bench(256, 64);
    };

    BENCHMARK("Stencil S=1024 D=128 (Medium)") {
        run_bench(1024, 128);
    };

    BENCHMARK("Stencil S=4096 D=256 (Large)") {
        run_bench(4096, 256);
    };
}
#endif
