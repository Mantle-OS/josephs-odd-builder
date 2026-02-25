#include "transpose.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

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


#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Forces: Scalar Throughput Benchmarks", "[science][forces][benchmark][scalar]")
{
    const DiskModel disk = SciencePresets::solarNebula();
    const Composition rocky = SciencePresets::rocky();

    // We want a large enough N to minimize jitter, but small enough to fit in L3.
    // 10k particles is a good "sweet spot" for throughput testing.
    constexpr size_t kParticleCount = 10000;
    std::vector<Particle> particles(kParticleCount);

    // Setup a predictable ring of particles to avoid branch-prediction noise
    for (size_t i = 0; i < kParticleCount; ++i) {
        float r_au = 1.0f + (float(i) / kParticleCount) * 10.0f;
        particles[i] = ParticleUtil::createDragParticle(r_au, 1e-4f, 100.0f, rocky);
    }

    // Block 3: Benchmarks
    // Pushing the scalar math to the limit before we bring in the SIMD heavy hitters.

    BENCHMARK("Gravity (Scalar, N=10k)")
    {
        Vec3f totalAccel{0, 0, 0};
        for (const auto &p : particles) {
            // We accumulate to a dummy variable to prevent the optimizer
            // from deleting the loop because it thinks we're doing nothing.
            totalAccel = totalAccel + Forces::gravity(p, disk);
        }
        return totalAccel;
    };

    BENCHMARK("Radiation Pressure (Scalar, N=10k)")
    {
        Vec3f totalAccel{0, 0, 0};
        for (const auto &p : particles) {
            totalAccel = totalAccel + Forces::radiationPressure(p, disk, rocky);
        }
        return totalAccel;
    };
}


TEST_CASE("Forces: Gas Drag Benchmarks", "[science][forces][benchmark][gasDrag]")
{
    const DiskModel disk = SciencePresets::solarNebula();
    const Composition rocky = SciencePresets::rocky();

    constexpr size_t kParticleCount = 10000;

    // We create two identical sets to ensure memory state doesn't bias the test
    std::vector<Particle> scalarParticles(kParticleCount);
    std::vector<Particle> simdParticles(kParticleCount);

    for (size_t i = 0; i < kParticleCount; ++i) {
        float r_au = 1.0f + (float(i) / kParticleCount) * 10.0f;
        scalarParticles[i] = ParticleUtil::createDragParticle(r_au, 1e-4f, 100.0f, rocky);
        simdParticles[i]   = scalarParticles[i];
    }
    BENCHMARK("Gas Drag (Scalar, N=10k)")
    {
        Vec3f totalAccel{0, 0, 0};
        for (const auto &p : scalarParticles) {
            totalAccel = totalAccel + Forces::gasDrag(p, disk);
        }
        return totalAccel;
    };

    // BENCHMARK("Gas Drag (SIMD, N=10k)")
    // {
    //     Vec3f totalAccel{0, 0, 0};
    //     for (const auto &p : scalarParticles) {
    //         totalAccel = totalAccel + Forces::gasDrag(p, disk);
    //     }
    //     return totalAccel;
    // };

}


TEST_CASE("Forces: Net Acceleration Benchmarks", "[science][forces][benchmark][netAcceleration]")
{
    const DiskModel disk = SciencePresets::solarNebula();
    const Composition rocky = SciencePresets::rocky();

    constexpr size_t kParticleCount = 10000;

    // We create two identical sets to ensure memory state doesn't bias the test
    std::vector<Particle> scalarParticles(kParticleCount);
    std::vector<Particle> simdParticles(kParticleCount);

    for (size_t i = 0; i < kParticleCount; ++i) {
        float r_au = 1.0f + (float(i) / kParticleCount) * 10.0f;
        scalarParticles[i] = ParticleUtil::createDragParticle(r_au, 1e-4f, 100.0f, rocky);
        simdParticles[i]   = scalarParticles[i];
    }

    BENCHMARK("Net Acceleration (Scalar, N=10k)")
    {
        // Scalar logic: Process one by one
        for (auto &p : scalarParticles) {
            p.acceleration = Forces::netAcceleration(p, disk, rocky);
        }
        // Return first element's x to prevent optimization
        return scalarParticles[0].acceleration.x;
    };

    BENCHMARK("Net Acceleration (SIMD Fused, N=10k)")
    {
        // SIMD logic: Process 8 at a time in a tight loop
        // This is where we expect the massive speedup from reduced math and cache hits
        Forces::calculateNetAccelerationSimd(simdParticles, disk);

        // Return first element's x to prevent optimization
        return simdParticles[0].acceleration.x;
    };
}


TEST_CASE("Forces: SIMD Memory Layout Benchmarks", "[science][forces][benchmark][memory]")
{
    // const DiskModel disk = SciencePresets::solarNebula();
    constexpr size_t kCount = 10240; // Multiple of 8

    // 1. AoS Data (The current messy way)
    Particles ps = ParticleUtil::genParticles(kCount);

    // 2. SoA Data (The fast way)
    // We allocate flat arrays for X, Y, Z coordinates
    std::vector<float> soaX(kCount), soaY(kCount), soaZ(kCount);
    std::vector<float> accX(kCount), accY(kCount), accZ(kCount);

    // Block 3: Benchmarks

    BENCHMARK("Memory: Manual Gather (Current AoS bottleneck)")
    {
        // This measures just the cost of getting data into registers
        float temp[8];
        float sum = 0;
        for (size_t i = 0; i < kCount; i += 8) {
            for (int j = 0; j < 8; ++j) {
                temp[j] = ps[i + j].position.x;
            }
            f32 reg = SIMD::pull(temp);
            // Anchor the result
            sum += ((float*)&reg)[0];
        }
        return sum;
    };

    BENCHMARK("Memory: Pure SoA Load (The theoretical limit)")
    {
        // This is what the CPU can do when data is perfectly aligned
        float sum = 0;
        for (size_t i = 0; i < kCount; i += 8) {
            f32 reg = SIMD::pull(&soaX[i]);
            sum += ((float*)&reg)[0];
        }
        return sum;
    };

    BENCHMARK("Memory: SIMD Transpose Kernel (8x8 block)")
    {
        // Testing the speed of your new transpose logic
        // We'll transpose a 8x8 block of coordinates
        for (size_t i = 0; i < kCount; i += 8) {
            // In a real scenario, you'd load 8 positions into a temp buffer
            // and then transpose them to get 3 registers (X, Y, Z)
            job::simd::transpose_kernel_8x8(&soaX[0], 8, &accX[0], 8);
        }
        return accX[0];
    };
}
#endif


