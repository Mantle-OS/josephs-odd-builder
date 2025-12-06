#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <random>

#include <ctx/job_stealing_ctx.h>

#include <view.h>
#include <bh_adapter.h>

using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;

TEST_CASE("Barnes-Hut Adapter: Basic Gravity", "[ai][bh][usage]") {
    JobStealerCtx ctx(4);

    BhConfig cfg;
    cfg.theta = 0.0f; // Force Exact Calculation (0.0 = Treat everyone as individual)
    BhAdapter adapter(cfg);

    // int B = 1;
    int S = 2;
    int D = 4;

    std::vector<float> input(S*D, 0.0f);
    // P1 at 0, P2 at 2.0
    input[1*D] = 2.0f;

    ViewR view(input.data(), ViewR::Extent{1, 2});
    ViewR out(input.data(), ViewR::Extent{1, 2});
    std::vector<float> outputData(S*D, 0.0f);
    ViewR output(outputData.data(), ViewR::Extent{1, 2});

    AttentionShape shape{1, 2, static_cast<uint32_t>(D), 1};
    AdapterCtx aCtx;

    adapter.adapt(*ctx.pool, shape, view, view, view, output, aCtx);

    // Same physics as FMM: Force ~ 0.25
    // BH Solver adds epsilon usually, so might be slightly less than 0.25
    CHECK(outputData[0] > 0.2f);
    CHECK(outputData[0] < 0.26f);
}


TEST_CASE("Barnes-Hut Adapter: Scaling Benchmark", "[ai][bh][bench]") {
    JobStealerCtx ctx(16);

    BhConfig cfg;
    cfg.theta = 0.5f; // Standard approximation
    BhAdapter adapter(cfg);

    AdapterCtx aCtx;
    const int D = 4; // Pos(3) + Mass(1)

    auto run_bench = [&](int n) {
        std::vector<float> data(n * D);
        // Galaxy distribution (cluster around 0)
        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, 50.0f);
        for(auto& x : data)
            x = dist(gen);

        ViewR view(data.data(), ViewR::Extent{1, static_cast<uint32_t>(n)});
        // Use same view for Source/Target/Output (Self-Gravity)
        ViewR out(data.data(), ViewR::Extent{1, static_cast<uint32_t>(n)});

        AttentionShape shape{1, static_cast<uint32_t>(n), static_cast<uint32_t>(D), 1};

        adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);
    };

    BENCHMARK("BH N=1024") { run_bench(1024); };
    BENCHMARK("BH N=4096") { run_bench(4096); };
    BENCHMARK("BH N=16384") { run_bench(16384); };
}


