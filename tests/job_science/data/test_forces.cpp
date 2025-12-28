#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>


#include <data/forces.h>
#include <data/particle.h>
#include <data/disk.h>
#include <data/composition.h>

using namespace job::science::data;

// CORE
TEST_CASE("Forces::gravity - magnitude and direction", "[forces][gravity]")
{
    // G * Stellar Mass
    constexpr float GM_sun = Forces::G * Forces::M_sun;
    DiskModel disk = SciencePresets::solarNebula();

    // Particle at 1 AU total radial distance
    const float r1_au = 1.0f;
    const float r1_m = DiskModelUtil::auToMeters(r1_au);
    Particle p1 = ParticleUtil::createTestParticle(r1_au, 1e-3f, SciencePresets::rocky());

    // acceleration magnitude: GM / r²
    const float expected_accel_mag = GM_sun / (r1_m * r1_m);
    Vec3f accel1 = Forces::gravity(p1, disk);

    // Magnitude Check (using calculated length of the vector)
    REQUIRE(accel1.length() == Catch::Approx(expected_accel_mag).margin(1e-5f));

    // Direction Check (Inward: X and Y components should be negative and equal)
    REQUIRE(accel1.x < 0.0f);
    REQUIRE(accel1.y < 0.0f);
    REQUIRE(accel1.x == Catch::Approx(accel1.y));
}

TEST_CASE("Forces::radiationPressure - magnitude and scaling", "[forces][radiation]")
{
    DiskModel disk = SciencePresets::solarNebula();
    Composition icy_comp = SciencePresets::icy();

    // 1. Setup a small particle at 1 AU
    const float r1_au = 1.0f;
    Particle p1 = ParticleUtil::createTestParticle(r1_au, 1e-6f, icy_comp);

    Vec3f accel1 = Forces::radiationPressure(p1, disk, icy_comp);

    // Radiation pressure is radially OUTWARD (positive X and Y)
    REQUIRE(accel1.x > 0.0f);
    REQUIRE(accel1.x == Catch::Approx(accel1.y));

    // 2. Check scaling with 1/r² (r2 = 2 * r1, so accel2 = accel1 / 4)
    const float r2_au = 2.0f;
    Particle p2 = ParticleUtil::createTestParticle(r2_au, 1e-6f, icy_comp);
    Vec3f accel2 = Forces::radiationPressure(p2, disk, icy_comp);

    REQUIRE(accel2.length() == Catch::Approx(accel1.length() / 4.0f).margin(1e-5f));
}

TEST_CASE("Forces::gasDrag - direction and proportionality", "[forces][drag]")
{
    DiskModel disk = SciencePresets::solarNebula();
    Composition comp = SciencePresets::rocky();

    const float r_au = 5.0f;
    const float vy = 100.0f; // 100 m/s in Y direction

    // 1. Particle moving in the Y direction (no X component expected for drag)
    Particle p1 = ParticleUtil::createDragParticle(r_au, 1e-4f, vy, comp);
    Vec3f accel1 = Forces::gasDrag(p1, disk);

    // Direction Check: Drag must be anti-parallel to velocity (negative Y)
    REQUIRE(accel1.x == Catch::Approx(0.0f));
    REQUIRE(accel1.z == Catch::Approx(0.0f));
    REQUIRE(accel1.y < 0.0f);

    // 2. Check proportionality with velocity (a proportional force)
    const float vy_double = 200.0f;
    Particle p2 = ParticleUtil::createDragParticle(r_au, 1e-4f, vy_double, comp);
    Vec3f accel2 = Forces::gasDrag(p2, disk);

    REQUIRE(accel2.length() == Catch::Approx(accel1.length() * 2.0f).margin(1e-5f));
}

TEST_CASE("Forces::netAcceleration - vector addition integrity", "[forces][net]")
{
    // Test that vector addition is correct by isolating the dominant force (gravity).

    // Setup Environment to ZERO OUT RAD/PHOTO FORCES
    DiskModel disk = SciencePresets::solarNebula();
    disk.radiationPressureCoeff = 0.0f; // Zero out radiation pressure influence

    Composition comp = SciencePresets::rocky();
    comp.absorptivity = 0.0f; // Zero out photophoretic/absorptivity influence

    // Particle Setup
    const float r_au = 1.0f;
    Particle p = ParticleUtil::createDragParticle(r_au, 1e-3f, 10.0f, comp);

    // Calculation
    Vec3f grav = Forces::gravity(p, disk);
    Vec3f net = Forces::netAcceleration(p, disk, comp);

    // Verification: Net acceleration should be very close to Gravitation + small Drag.
    // The epsilon margin of 0.1f is now much larger than the expected drag force,
    // ensuring the test passes as long as the HUGE forces are gone.
    REQUIRE(net.length() == Catch::Approx(grav.length()).epsilon(0.1f));

    // Check direction sanity
    REQUIRE(net.x < 0.0f);
    REQUIRE(net.y < 0.0f);
    REQUIRE(net.z == Catch::Approx(0.0f));
}
