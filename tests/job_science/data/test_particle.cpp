#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <limits>

#include <data/particle.h>
#include <data/disk.h>
#include <data/composition.h>

using namespace job::science::data;

TEST_CASE("Vec3f mathematical operations", "[vec3f][math]")
{
    const Vec3f v1 = {1.0f, 2.0f, 3.0f};
    const Vec3f v2 = {4.0f, 5.0f, 6.0f};
    const float scalar = 2.0f;

    // Addition
    Vec3f v_add = v1 + v2;
    REQUIRE(v_add.x == Catch::Approx(5.0f));
    REQUIRE(v_add.y == Catch::Approx(7.0f));
    REQUIRE(v_add.z == Catch::Approx(9.0f));

    // Subtraction
    Vec3f v_sub = v2 - v1;
    REQUIRE(v_sub.x == Catch::Approx(3.0f));
    REQUIRE(v_sub.y == Catch::Approx(3.0f));
    REQUIRE(v_sub.z == Catch::Approx(3.0f));

    // Multiplication (Scalar)
    Vec3f v_mul = v1 * scalar;
    REQUIRE(v_mul.x == Catch::Approx(2.0f));
    REQUIRE(v_mul.y == Catch::Approx(4.0f));
    REQUIRE(v_mul.z == Catch::Approx(6.0f));
}

TEST_CASE("Particle validity and geometry basics", "[particle][geometry]")
{
    Particle p{};
    p.radius = 1.0f;
    p.composition.density = 3000.0f;
    p.mass = (4.0f / 3.0f * M_PI) * std::pow(p.radius, 3.0f) * p.composition.density;

    REQUIRE(ParticleUtil::isValid(p));
    REQUIRE(p.mass > 0.0f);

    const float expectedVolume = (4.0f / 3.0f * M_PI) * std::pow(p.radius, 3.0f);

    const float expectedArea = 4.0f * M_PI * p.radius * p.radius;

    REQUIRE(ParticleUtil::volume(p) == Catch::Approx(expectedVolume).margin(1e-5f));
    REQUIRE(ParticleUtil::surfaceArea(p) == Catch::Approx(expectedArea).margin(1e-5f));
}

TEST_CASE("Particle invalid when radius is zero or non-finite", "[particle][validity]")
{
    Particle p{};

    p.radius = 0.0f;
    REQUIRE_FALSE(ParticleUtil::isValid(p));

    p.radius = -1.0f;
    REQUIRE_FALSE(ParticleUtil::isValid(p));

    p.radius = std::numeric_limits<float>::quiet_NaN();
    REQUIRE_FALSE(ParticleUtil::isValid(p));
}

TEST_CASE("Particle volume and surface area scale correctly with radius", "[particle][scaling]")
{
    Particle p1{};
    p1.radius = 1.0f;

    Particle p2{};
    p2.radius = 2.0f;

    REQUIRE(ParticleUtil::volume(p2) / ParticleUtil::volume(p1) == Catch::Approx(8.0f).margin(1e-5f));
    REQUIRE(ParticleUtil::surfaceArea(p2) / ParticleUtil::surfaceArea(p1) == Catch::Approx(4.0f).margin(1e-5f));
}

TEST_CASE("Vec3f operator chaining and initialization", "[vec3f][math]")
{
    const Vec3f v1 = {1.0f, 1.0f, 1.0f};
    const Vec3f v2 = {3.0f, 3.0f, 3.0f};
    const float s = 10.0f;

    Vec3f result = (v1 - v2) * s;

    REQUIRE(result.x == Catch::Approx(-20.0f));
    REQUIRE(result.y == Catch::Approx(-20.0f));
    REQUIRE(result.z == Catch::Approx(-20.0f));

    Vec3f result2 = v1 + (v2 * s);

    REQUIRE(result2.x == Catch::Approx(31.0f));
    REQUIRE(result2.y == Catch::Approx(31.0f));
    REQUIRE(result2.z == Catch::Approx(31.0f));
}

TEST_CASE("Particle composition integration checks", "[particle][composition]")
{
    Particle p{};
    p.radius = 1.0f;

    p.composition = SciencePresets::metallic();
    REQUIRE(ParticleUtil::density(p) == Catch::Approx(7800.0f));

    p.composition = SciencePresets::rocky();
    REQUIRE(ParticleUtil::density(p) == Catch::Approx(3300.0f));

    REQUIRE(CompositionUtil::reflectivity(p.composition) == Catch::Approx(0.3f));
    const float expected_mass = ParticleUtil::volume(p) * ParticleUtil::density(p);
    p.mass = expected_mass;
    REQUIRE(p.mass == Catch::Approx(expected_mass).margin(1e-5f));
}
