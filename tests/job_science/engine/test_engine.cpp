#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <atomic>

#include <engine/engine.h>
#include <data/disk.h>
#include <data/zones.h>
#include <data/forces.h>
#include <data/composition.h>
#include <data/real_type.h>

using namespace pps::engine;
using namespace pps::data;
using namespace std::chrono_literals;

constexpr real_t kTestTimeStep = static_cast<real_t>(60.0f); // 1 minute

TEST_CASE("Engine Initialization and Configuration", "[engine][init]")
{
    DiskModel disk = presets::solarNebula();
    Zones zones = presets::solarSystemLike();

    Engine engine(disk, zones, 10);

    REQUIRE(engine.particles().size() == 10);
    REQUIRE_FALSE(engine.running());
    REQUIRE_FALSE(engine.pause());

    REQUIRE(engine.disk().innerRadius == disk.innerRadius);
    REQUIRE(engine.zones().midBoundaryAU == zones.midBoundaryAU);

    bool hasMass = false;
    for(const auto& p : engine.particles()) {
        if (p.mass > static_cast<real_t>(0.0f)) {
            hasMass = true;
            break;
        }
    }
    REQUIRE(hasMass);
}

TEST_CASE("Engine Start, Stop, and Pause Control", "[engine][control]")
{
    DiskModel disk = presets::solarNebula();
    Zones zones = presets::solarSystemLike();
    Engine engine(disk, zones, 10);

    engine.start();
    REQUIRE(engine.running());

    REQUIRE(engine.pause());
    REQUIRE(engine.running());
    REQUIRE(engine.play());
    REQUIRE(engine.running());
    engine.stop();

    std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(engine.running());
}

TEST_CASE("Engine Step and Velocity Verlet Integration Integrity", "[engine][step][sync]")
{
    DiskModel disk = presets::solarNebula();
    Zones zones = presets::solarSystemLike();

    Engine engine(disk, zones, 1);
    engine.step(kTestTimeStep);

    Particle p_start = engine.particles()[0];

    engine.step(kTestTimeStep);
    Particle p_end = engine.particles()[0];

    // from what I read (?Keplerian?) ... Not really sure
    REQUIRE(p_end.velocity.length() > static_cast<real_t>(0.0f));
    REQUIRE_FALSE(p_end.position.x == Catch::Approx(p_start.position.x));

    REQUIRE(p_end.zone != DiskZone::Inner_Disk);
}

TEST_CASE("Engine Callback Notification on Step", "[engine][callback]")
{
    DiskModel disk = presets::solarNebula();
    Zones zones = presets::solarSystemLike();
    Engine engine(disk, zones, 5);

    std::atomic<int> calls{0};

    engine.setParticleCallback([&](const std::vector<Particle>& parts) {
        calls++;
        REQUIRE(parts.size() == 5);
    });
    // 2x's a charm I guess
    engine.step(kTestTimeStep);
    engine.step(kTestTimeStep);

    REQUIRE(calls.load() == 2);
}
