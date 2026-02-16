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

    REQUIRE(job::core::isSafeFinite(t_expected));          // or std::isfinite(t_expected)
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
