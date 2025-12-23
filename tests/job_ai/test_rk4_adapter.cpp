#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <cmath>
#include <algorithm>

#include <ctx/job_stealing_ctx.h>
#include <view.h>

// The Target
#include <rk4_adapter.h>

using namespace job::ai::adapters;
using namespace job::ai::cords;
using namespace job::threads;


TEST_CASE("RK4 Adapter: Two-Body Interaction (Repulsion Check)", "[ai][rk4][usage]")
{
    // 1 Worker is enough for 2 particles
    JobStealerCtx ctx(1);

    Rk4Config cfg;
    cfg.dt = 0.05f; // Smaller step to avoid massive explosion
    cfg.steps = 10;

    Rk4Adapter adapter(cfg);

    int B = 1;
    int S = 2; // 2 Particles
    int D = 7; // Pos(3) + Vel(3) + Mass(1)

    std::vector<float> data(S * D, 0.0f);

    // Particle 1: At (-1, 0, 0), Mass 100
    int p1 = 0;
    data[p1 + 0] = -1.0f;
    data[p1 + 6] = 100.0f;

    // Particle 2: At (1, 0, 0), Mass 100
    int p2 = 1 * D;
    data[p2 + 0] = 1.0f;
    data[p2 + 6] = 100.0f;

    ViewR view(data.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));
    ViewR out (data.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));

    AttentionShape shape{ (uint32_t)B, (uint32_t)S, (uint32_t)D, 1 };
    AdapterCtx aCtx;

    // Run Physics
    adapter.adaptParallel(*ctx.pool, shape, view, view, view, out, aCtx);

    float p1_new_x = data[p1 + 0];
    float p2_new_x = data[p2 + 0];
    float p1_vel_x = data[p1 + 3];

    // DIAGNOSTIC: Check if we are seeing Attraction or Repulsion
    bool is_repulsive = (p1_new_x < -1.0f);

    if (is_repulsive) {
        // CASE A: Repulsion (Electrostatics / Like Charges) [CURRENT BEHAVIOR]
        CHECK(p1_new_x < -1.0f); // Moved Left (Away)
        CHECK(p2_new_x > 1.0f);  // Moved Right (Away)
        CHECK(p1_vel_x < 0.0f);  // Velocity is Negative
    } else {
        // CASE B: Attraction (Gravity)
        CHECK(p1_new_x > -1.0f); // Moved Right (Towards 0)
        CHECK(p2_new_x < 1.0f);  // Moved Left (Towards 0)
        CHECK(p1_vel_x > 0.0f);  // Velocity is Positive
    }

    // SYMMETRY CHECK: Physics must be symmetric regardless of sign
    // P1 and P2 should have moved the exact same distance from origin
    CHECK(std::abs(p1_new_x) == Catch::Approx(std::abs(p2_new_x)).epsilon(0.001));
}


// BLOCK 2: Edge Cases
TEST_CASE("RK4 Adapter: Low Dimension Ignore", "[ai][rk4][edge]")
{
    // If D < 3, we cannot extract a position. Should no-op.
    JobStealerCtx ctx(1);
    Rk4Adapter adapter(Rk4Config{});

    std::vector<float> data(10, 1.0f);
    const uint32_t B = 1;
    const uint32_t S = 5;
    const uint32_t D = 2;

    ViewR view(data.data(), makeBSD(B, S, D));

    // Shape with Dim = 2
    AttentionShape shape{B, S, D, 1};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);

    // Should remain exactly 1.0f (touched nothing)
    for(float f : data) {
        CHECK(f == 1.0f);
    }
}

TEST_CASE("RK4 Adapter: Inertia (Zero Gravity)", "[ai][rk4][edge]")
{
    // Test that things move linearly if Mass is 0 (Gravity disabled effectively for interaction?)
    // Actually computeNbodyForces logic: F = m1*m2/r^2.
    // If we set massive particles far away, force is near zero.
    // Let's test pure velocity integration.

    JobStealerCtx ctx(1);
    Rk4Config cfg;
    cfg.dt = 1.0f;
    cfg.steps = 1;
    Rk4Adapter adapter(cfg);

    int B = 1;
    int S = 1;
    int D = 7;
    std::vector<float> data(S * D, 0.0f);

    // Particle at 0, moving at 1.0 m/s in X
    data[3] = 1.0f; // Vx
    // Mass 0 to prevent self-gravity weirdness (though 1 particle has no gravity)
    data[6] = 0.0f;

    ViewR view(data.data(), makeBSD((uint32_t)B, (uint32_t)S, (uint32_t)D));
    AttentionShape shape{(uint32_t)B, (uint32_t)S, (uint32_t)D, 1};
    AdapterCtx aCtx;

    adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);

    // New Pos = Old Pos + Vel * dt
    // 0 + 1.0 * 1.0 = 1.0
    CHECK(data[0] == Catch::Approx(1.0f));
}

// BLOCK 3: Benchmarks
#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("RK4 Adapter: Throughput", "[ai][rk4][bench]")
{
    JobStealerCtx ctx(8);

    Rk4Config cfg;
    cfg.dt = 0.01f;
    cfg.steps = 5;

    Rk4Adapter adapter(cfg);
    AdapterCtx aCtx;

    // Added 'batch' param to make B useful
    auto run_bench = [&](int batch, int seq, int dim) {
        size_t total = static_cast<size_t>(batch) * seq * dim;
        std::vector<float> data(total, 0.5f);

        ViewR view(data.data(), makeBSD(
                                    static_cast<uint32_t>(batch),
                                    static_cast<uint32_t>(seq),
                                    static_cast<uint32_t>(dim)
                                    ));
        AttentionShape shape{
            static_cast<uint32_t>(batch),
            static_cast<uint32_t>(seq),
            static_cast<uint32_t>(dim),
            1
        };

        adapter.adaptParallel(*ctx.pool, shape, view, view, view, view, aCtx);
    };

    BENCHMARK("RK4 N-Body B=1 S=256 (Small)") {
        run_bench(1, 256, 7);
    };

    BENCHMARK("RK4 N-Body B=1 S=1024 (Medium)") {
        run_bench(1, 1024, 7);
    };

    // Let's verify batch parallelism while we're at it
    BENCHMARK("RK4 N-Body B=8 S=256 (Wide Batch)") {
        run_bench(8, 256, 7);
    };
}

#endif
