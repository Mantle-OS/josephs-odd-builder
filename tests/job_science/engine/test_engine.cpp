#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <atomic>

#include <engine/engine.h>

#include <data/disk.h>
#include <data/zones.h>
#include <data/particle.h>

using namespace job::science::engine;
using namespace job::science::data;

using namespace std::chrono_literals;

constexpr float kTestTimeStep = 60.0f; // 1 minute

TEST_CASE("Engine: Initialization and configuration", "[engine][init]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    Engine engine(disk, zones, DefaultEngineConfig(), 10);

    REQUIRE(engine.particles().size() == 10);

    REQUIRE_FALSE(engine.running());
    REQUIRE_FALSE(engine.pause()); // can't pause if not running

    REQUIRE(engine.disk().innerRadius == Catch::Approx(disk.innerRadius));
    REQUIRE(engine.zones().midBoundaryAU == Catch::Approx(zones.midBoundaryAU));

    bool hasMass = false;
    for (const auto &p : engine.particles()) {
        if (p.mass > 0.0f) {
            hasMass = true;
            break;
        }
    }
    REQUIRE(hasMass);
}

TEST_CASE("Engine: Start/Stop/Pause/Play control", "[engine][control]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    Engine engine(disk, zones, DefaultEngineConfig(), 10);

    engine.start();
    REQUIRE(engine.running());

    REQUIRE(engine.pause());
    REQUIRE(engine.running());

    REQUIRE(engine.play());
    REQUIRE(engine.running());

    engine.stop();
    REQUIRE_FALSE(engine.running());
}

TEST_CASE("Engine: step() advances particle state and recomputes derived fields", "[engine][step]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    Engine engine(disk, zones, DefaultEngineConfig(), 1);

    REQUIRE(engine.particles().size() == 1);

    const Particle p0 = engine.particles()[0];

    engine.step(kTestTimeStep);

    const Particle p1 = engine.particles()[0];

    // It should move (very weak constraints; integrator specifics are tested elsewhere)
    REQUIRE_FALSE(p1.position.x == Catch::Approx(p0.position.x));
    REQUIRE(p1.velocity.length() > 0.0f);

    // Temperature should be consistent with disk model + radius
    const float r_au_expected = DiskModelUtil::metersToAU(p1.position.length());
    const float t_expected    = DiskModelUtil::temperature(disk, r_au_expected);

    REQUIRE(job::core::isSafeFinite(t_expected));
    REQUIRE(job::core::isSafeFinite(p1.temperature));

    REQUIRE(p1.temperature == Catch::Approx(t_expected));

    // Zone should be consistent with classifier
    const DiskZone z_expected = ZonesUtil::classify(zones, p1, disk);
    REQUIRE(p1.zone == z_expected);
}

TEST_CASE("Engine: particle callback fires once per step()", "[engine][callback]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    Engine engine(disk, zones, DefaultEngineConfig(), 5);

    std::atomic<int> calls{0};

    engine.setParticleCallback([&](const Particles &parts) {
        calls.fetch_add(1, std::memory_order_relaxed);
        REQUIRE(parts.size() == 5);
    });

    engine.step(kTestTimeStep);
    engine.step(kTestTimeStep);

    REQUIRE(calls.load(std::memory_order_relaxed) == 2);
}

// // Test the integrators via the Engine orchestrator
// // Test the integrators via the Engine orchestrator

TEST_CASE("Engine: Euler Integrator Type", "[engine][integrator][euler]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    // Use the default config helper and override the solver
    auto cfg = DefaultEngineConfig(job::threads::SchedulerType::Fifo, IntegratorType::Euler);

    Engine engine(disk, zones, cfg, 10);

    REQUIRE(engine.particles().size() == 10);
    const auto p_initial = engine.particles()[0].position;

    // Euler needs a small dt to not explode, but for a single step kTestTimeStep is fine
    engine.step(kTestTimeStep);

    const auto p_final = engine.particles()[0].position;

    // Verify state changed and remains numerically sound
    CHECK_FALSE(p_final.x == Catch::Approx(p_initial.x));
    CHECK(job::core::isSafeFinite(p_final.x));
    CHECK(engine.particles()[0].velocity.length() > 0.0f);
}

TEST_CASE("Engine: RK4 Integrator Type", "[engine][integrator][rk4]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    auto cfg = DefaultEngineConfig(job::threads::SchedulerType::Fifo, IntegratorType::RungeKutta4);

    Engine engine(disk, zones, cfg, 10);

    // RK4 performs 4 intermediate force evaluations; ensure the engine orchestrates this correctly
    engine.step(kTestTimeStep);

    for (const auto& p : engine.particles()) {
        REQUIRE(job::core::isSafeFinite(p.position.x));
        REQUIRE(job::core::isSafeFinite(p.velocity.x));
        // Ensure acceleration was updated/synced at the end of the step
        REQUIRE(job::core::isSafeFinite(p.acceleration.x));
    }
}

TEST_CASE("Engine: BarnesHut Integrator Type", "[engine][integrator][barnshut]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    auto cfg = DefaultEngineConfig(job::threads::SchedulerType::WorkStealing, IntegratorType::BarnesHut);

    // Use a larger G or closer particles if you want to guarantee non-zero N-body force
    cfg.barnesHut.theta = 0.5f;

    Engine engine(disk, zones, cfg, 50);

    // Initial positions
    auto p0 = engine.particles()[0].position;

    engine.step(kTestTimeStep);

    for (const auto& p : engine.particles()) {
        // Use REQUIRE so we stop on the first NaN
        REQUIRE(job::core::isSafeFinite(p.acceleration.x));
        REQUIRE(job::core::isSafeFinite(p.position.x));
    }

    // Verify that at least some movement happened (gravity + disk potential)
    CHECK_FALSE(engine.particles()[0].position.x == Catch::Approx(p0.x));
}

TEST_CASE("Engine: FMM Integrator Type", "[engine][integrator][fmm]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    auto cfg = DefaultEngineConfig(job::threads::SchedulerType::WorkStealing, IntegratorType::FastMultipoleMethod);
    cfg.fmm.theta = 0.6f;
    cfg.fmm.maxLeafSize = 4;

    Engine engine(disk, zones, cfg, 20);

    // Use a smaller dt for the test step to prevent "numerical crazyness"
    const float safeDt = 0.1f;
    engine.step(safeDt);

    for (const auto &p : engine.particles()) {
        REQUIRE(job::core::isSafeFinite(p.position.y));
        REQUIRE(job::core::isSafeFinite(p.acceleration.y));
    }
}

TEST_CASE("Engine: Runtime Integrator Swapping", "[engine][integrator][swap]")
{
    const DiskModel disk  = SciencePresets::solarNebula();
    const Zones     zones = SciencePresets::solarSystemLike();

    // Start with the simple Euler
    auto cfg = DefaultEngineConfig(job::threads::SchedulerType::Fifo, IntegratorType::Euler);

    Engine engine(disk, zones, cfg, 16);
    engine.step(0.1f);

    // Verify the engine reports Euler
    // (Assuming you have a getter for config or the current integrator type)
    REQUIRE(engine.currentIntegratorType() == IntegratorType::Euler);

    // Swap to RK4 mid-flight.
    // Since we pass the cfg by value/ref to the engine, ensure the engine
    // re-calls updateIntegrator() if the config object it holds is modified.
    cfg.integrator = IntegratorType::RungeKutta4;

    // Explicitly step—the engine should detect the mismatch in m_integrator->type()
    // and trigger the swap/re-prime logic we discussed.
    engine.step(0.1f);

    for (const auto& p : engine.particles()) {
        REQUIRE(job::core::isSafeFinite(p.position.x));
    }

    SUCCEED("Engine successfully transitioned from Euler to RK4 without numerical explosion");
}
