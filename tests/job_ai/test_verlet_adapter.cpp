#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <random>

#include <job_thread_pool.h>
#include <ctx/job_stealing_ctx.h>

#include <verlet_adapter.h>
#include <verlet_config.h>

using namespace job::ai;
using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;
using job::ai::cords::makeBSD;


TEST_CASE("Verlet Adapter: Kinematics (Drift)", "[ai][verlet][usage]") {
    JobStealerCtx ctx(4);

    VerletConfig cfg;
    cfg.dt = 1.0f; // 1 second steps for easy math
    VerletAdapter adapter(cfg);

    // 1 Token, Dim 6 (Pos + Vel)
    // Pos = (0,0,0), Vel = (10, 0, 0)
    // After 1s, Pos should be (10, 0, 0)
    int D = 6;
    std::vector<float> data(D, 0.0f);
    data[3] = 10.0f; // Vx

    ViewR view(data.data(), makeBSD(1u, 1u, static_cast<uint32_t>(D)));
    AttentionShape shape{1u, 1u, static_cast<uint32_t>(D), 1u};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);

    CHECK(data[0] == Catch::Approx(10.0f)); // X moved
    CHECK(data[3] == Catch::Approx(10.0f)); // Vx preserved
}

TEST_CASE("Verlet Adapter: N-Body Gravity", "[ai][verlet][usage]") {
    JobStealerCtx ctx(4);
    VerletAdapter adapter; // dt=0.01

    // Two particles falling towards each other
    // P1: (-1, 0, 0)
    // P2: ( 1, 0, 0)
    // Velocities 0.
    int S = 2;
    int D = 7; // Pos(3) + Vel(3) + Mass(1)
    std::vector<float> data(S * D, 0.0f);

    // P1
    data[0] = -1.0f;
    data[6] = 1000.0f; // Heavy Mass

    // P2
    data[D + 0] = 1.0f;
    data[D + 6] = 1000.0f; // Heavy Mass

    ViewR view(data.data(), makeBSD(1u, static_cast<uint32_t>(S), static_cast<uint32_t>(D)));
    AttentionShape shape{1u, static_cast<uint32_t>(S), static_cast<uint32_t>(D), 1u};
    AdapterCtx aCtx;

    // Run 10 steps
    for(int i=0; i <10; ++i)
        adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);

    // Check convergence
    // P1 should move Right (> -1.0)
    // P2 should move Left (< 1.0)
    CHECK(data[0] > -1.0f);
    CHECK(data[D + 0] < 1.0f);

    // Symmetry check
    CHECK(data[0] == Catch::Approx(-data[D + 0]));
}

TEST_CASE("Verlet Adapter: Information Gravity (Value Mixing)", "[ai][verlet][mixing]") {
    JobStealerCtx ctx(4);
    VerletConfig cfg;
    cfg.dt = 0.1f;
    VerletAdapter adapter(cfg);

    const int S = 2;
    const int D = 8;

    // Using separate buffers to emulate the "Ping Pong" logic manually
    std::vector<float> data(S * D, 0.0f);

    // --- P1 (Listener) ---
    data[0] = -1.0f;
    data[6] = 1.0f;  // Light
    data[7] = 0.0f;  // Empty Mind

    // --- P2 (Speaker) ---
    data[D + 0] = 1.0f;
    data[D + 6] = 100.0f; // Heavy
    data[D + 7] = 10.0f;  // The Secret

    AttentionShape shape{1u, (uint32_t)S, (uint32_t)D, 1u};
    AdapterCtx aCtx;

    std::vector<float> outData = data;
    ViewR inputView(data.data(), makeBSD(1u, (uint32_t)S, (uint32_t)D));
    ViewR outputView(outData.data(), makeBSD(1u, (uint32_t)S, (uint32_t)D));

    // RUN STEP 1: P1 absorbs P2's value. P2 retains memory.
    // Position does NOT change yet (Velocity updated at end of step).
    adapter.adaptParallel(*ctx.pool, shape, inputView, inputView, inputView, outputView, aCtx);

    // SWAP: Output becomes Input
    std::memcpy(data.data(), outData.data(), data.size() * sizeof(float));

    // RUN STEP 2: Position updates based on new velocity. Mixing happens again.
    adapter.adaptParallel(*ctx.pool, shape, inputView, inputView, inputView, outputView, aCtx);

    // --- VERIFICATION ---

    // 1. Physics Check: P1 should move towards P2
    // Before: -1.0. After 2 steps: Should be > -1.0
    CHECK(outData[0] > -1.0f);

    // 2. AI Check: P1 (Listener)
    // Should have absorbed massive value from P2
    float p1_newValue = outData[7];
    INFO("P1 Value: " << p1_newValue);
    CHECK(p1_newValue > 10.0f);

    // 3. Asymmetry Check: P2 (Speaker)
    // P2 should NOT have forgotten its secret (10.0)
    // It should grow slightly (hearing P1) but stay close to 10-ish.
    float p2_newValue = outData[D + 7];
    INFO("P2 Value: " << p2_newValue);

    CHECK(p2_newValue >= 10.0f);
    CHECK(p2_newValue < 50.0f); // Stability check
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Verlet Adapter: Throughput (N-Body O(N^2)) 4 threads", "[ai][verlet][bench]") {
    JobStealerCtx ctx(4);
    VerletAdapter adapter;
    const int D = 64;
    AdapterCtx aCtx;
    aCtx.embedDim = D;
    auto run_bench = [&](int seq_len) {
        const int B = 1;
        const int S = seq_len;
        std::vector<float> data(B * S * D);
        std::mt19937 gen(42);
        std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
        for(auto &x : data)
            x = dist(gen);

        ViewR view(data.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D) );
        AttentionShape shape{ (uint32_t)B, (uint32_t)S, (uint32_t)D, 1 };

        adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);
    };


    BENCHMARK("Verlet N=1024") { run_bench(1024); };
    BENCHMARK("Verlet N=2048") { run_bench(2048); };
}
#endif
