#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
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

    ViewR view(data.data(), ViewR::Extent{1, 1});
    AttentionShape shape{1, 1, 6, 1};
    AdapterCtx aCtx;

    adapter.adapt(*ctx.pool, shape, view, view, view, view, aCtx);

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

    ViewR view(data.data(), ViewR::Extent{1, 2});
    AttentionShape shape{1, 2, static_cast<uint32_t>(D), 1};
    AdapterCtx aCtx;

    // Run 10 steps
    for(int i=0; i<10; ++i) {
        adapter.adapt(*ctx.pool, shape, view, view, view, view, aCtx);
    }

    // Check convergence
    // P1 should move Right (> -1.0)
    // P2 should move Left (< 1.0)
    CHECK(data[0] > -1.0f);
    CHECK(data[D + 0] < 1.0f);

    // Symmetry check
    CHECK(data[0] == Catch::Approx(-data[D + 0]));
}


TEST_CASE("Verlet Adapter: Throughput (N-Body O(N^2))", "[ai][verlet][bench]") {
    JobStealerCtx ctx(16);
    VerletAdapter adapter; // Default config
    AdapterCtx aCtx;

    // N=1024 particles
    // Interactions per step: 1024*1024 ~= 1 Million forces
    // Complexity: O(N^2)
    const int B = 1;
    const int S = 1024;
    const int D = 7; // Pos(3) + Vel(3) + Mass(1)

    std::vector<float> data(B * S * D);

    // Random initialization
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    for(auto& x : data) x = dist(gen);

    cords::ViewR view(data.data(), cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(S)});
    cords::AttentionShape shape{static_cast<uint32_t>(B), static_cast<uint32_t>(S), static_cast<uint32_t>(D), 1};

    BENCHMARK("Verlet Step (N=1024)") {
        adapter.adapt(*ctx.pool, shape, view, view, view, view, aCtx);
        return data[0]; // Output dependency
    };
}
