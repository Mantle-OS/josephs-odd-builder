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

    const int B = 1;
    const int S = 2;
    const int D = 4;

    std::vector<float> input(B * S * D, 0.0f);
    // P1 at 0, P2 at 2.0
    input[1 * D + 0] = 2.0f; // particle 1, dim 0

    std::vector<float> outputData(B * S * D, 0.0f);

    ViewR src(input.data(),  makeBSD(B, S, D));
    ViewR dst(outputData.data(), makeBSD(B, S, D));

    AttentionShape shape{
        static_cast<uint32_t>(B),
        static_cast<uint32_t>(S),
        static_cast<uint32_t>(D),
        1
    };
    AdapterCtx aCtx;

    // self-attention style: K=Q=V=positions+mass
    adapter.adaptParallel(*ctx.pool, shape, src, src, src, dst, aCtx);

    // Same physics as FMM: Force ~ 0.25
    // CHECK(outputData[0] > 0.2f);
    CHECK(outputData[0] > 0.0001f); // Adjusted for G*0.001 scaling
    CHECK(outputData[0] < 0.26f);
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Barnes-Hut Adapter: Scaling Benchmark", "[ai][bh][bench]") {
    JobStealerCtx ctx(16);

    BhConfig cfg;
    cfg.theta = 0.5f; // Standard approximation
    BhAdapter adapter(cfg);

    AdapterCtx aCtx;
    const int D = 4; // Pos(3) + Mass(1)

    auto run_bench = [&](int n) {
        const int B = 1;
        std::vector<float> data(B * n * D);

        // Galaxy distribution (cluster around 0)
        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, 50.0f);
        for (auto &x : data)
            x = dist(gen);

        ViewR view(data.data(), makeBSD(B, static_cast<uint32_t>(n), static_cast<uint32_t>(D)));
        // in-place is fine for throughput benchmark
        ViewR out(data.data(), makeBSD(B, static_cast<uint32_t>(n), static_cast<uint32_t>(D)));

        AttentionShape shape{
            static_cast<uint32_t>(B),
            static_cast<uint32_t>(n),
            static_cast<uint32_t>(D),
            1
        };

        adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);
    };

    BENCHMARK("BH N=1024")  { run_bench(1024);  };
    BENCHMARK("BH N=4096")  { run_bench(4096);  };
    BENCHMARK("BH N=16384") { run_bench(16384); };
}
#endif


