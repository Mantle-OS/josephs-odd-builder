#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <random>

#include <ctx/job_stealing_ctx.h>

#include <view.h>
#include <dense_adapter.h>

using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;


TEST_CASE("Dense Adapter: TEST Basic (Uniform)", "[ai][dense][usage]") {
    JobStealerCtx ctx(4);

    DenseConfig cfg;
    cfg.scale = 0.0f; // Forces Q*K^T to 0 -> Softmax becomes Uniform Distribution
    DenseAdapter adapter(cfg);

    const int B = 1;
    const int S = 2;
    const int D = 4;

    std::vector<float> input(B * S * D, 0.0f);
    // Token 0: [0, 0, 0, 0]
    // Token 1: [2, 0, 0, 0]
    input[1 * D + 0] = 2.0f;

    std::vector<float> outputData(B * S * D, 0.0f);

    ViewR src(input.data(),  makeBSD(B, S, D));
    ViewR dst(outputData.data(), makeBSD(B, S, D));

    AttentionShape shape{ (uint32_t)B, (uint32_t)S, (uint32_t)D, 1 };
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, src, src, src, dst, aCtx);

    // With uniform attention (0.5, 0.5):
    // Output = 0.5 * Token0 + 0.5 * Token1
    // Output = 0.5 * 0 + 0.5 * 2.0 = 1.0
    CHECK(outputData[0] == Catch::Approx(1.0f));
}



#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Dense Adapter: Pure Kernel Benchmark", "[ai][dense][bench]") {
    JobStealerCtx ctx(4);

    DenseConfig cfg;
    cfg.scale = 0.0f;
    DenseAdapter adapter(cfg);

    AdapterCtx aCtx;
    const int D = 4;
    const int B = 1;

    auto run_pure_bench = [&](int n) {
        // 1. SETUP (Outside the measurement)
        std::vector<float> data(B * n * D);
        std::vector<float> outData(B * n * D);

        std::mt19937 gen(42);
        std::normal_distribution<float> dist(0.0f, 50.0f);
        for (auto &x : data) x = dist(gen);

        ViewR view(data.data(), makeBSD(B, n, D));
        ViewR out(outData.data(), makeBSD(B, n, D));

        AttentionShape shape{ (uint32_t)B, (uint32_t)n, (uint32_t)D, 1 };

        // 2. MEASURE (Only the kernel)
        BENCHMARK("Dense Pure N=" + std::to_string(n)) {
            adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);
            return outData[0]; // Prevent compiler optimization
        };
    };

    run_pure_bench(1024);
    run_pure_bench(4096);
}
#endif


