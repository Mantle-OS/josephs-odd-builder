#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <data/disk.h>

using namespace job::science::data;

TEST_CASE("DiskModel AU to meters conversion", "[disk][constants]")
{
    // The core constant conversion must be correct
    const float AU_m = DiskModelUtil::auToMeters(1.0f);

    // 1 AU ≈ 1.495978707 × 10¹¹ m
    REQUIRE(AU_m == Catch::Approx(1.495978707e11f).margin(1e7f));
}

TEST_CASE("DiskModel presets are physically valid", "[disk][presets]")
{
    // Test validity of all built-in presets
    auto solar  = SciencePresets::solarNebula();
    auto tTauri = SciencePresets::tTauriDisk();
    auto cold   = SciencePresets::coldOuterDisk();

    REQUIRE(DiskModelUtil::isValid(solar));
    REQUIRE(DiskModelUtil::isValid(tTauri));
    REQUIRE(DiskModelUtil::isValid(cold));

    // Test essential geometrical constraints
    REQUIRE(solar.innerRadius < solar.outerRadius);
    REQUIRE(tTauri.innerRadius < tTauri.outerRadius);
    REQUIRE(cold.innerRadius   < cold.outerRadius);

    // Test that physics functions return positive values at 1 AU
    REQUIRE(DiskModelUtil::temperature(solar, 1.0f) > 0.0f);
    REQUIRE(DiskModelUtil::gasDensity(solar, 1.0f)  > 0.0f);
    REQUIRE(DiskModelUtil::pressure(solar, 1.0f)    > 0.0f);
}

TEST_CASE("DiskModel temperature decreases with radius", "[disk][gradient]")
{
    // Temperature must follow the power law T ∝ r^-0.5
    auto disk = SciencePresets::solarNebula();

    float t1 = DiskModelUtil::temperature(disk, 0.5f);
    float t2 = DiskModelUtil::temperature(disk, 1.0f);
    float t3 = DiskModelUtil::temperature(disk, 5.0f);

    REQUIRE(t1 > t2);
    REQUIRE(t2 > t3);
}

TEST_CASE("DiskModel density and pressure profiles are consistent", "[disk][physics]")
{
    // Density and Pressure must follow the power law, decreasing with radius
    auto disk = SciencePresets::tTauriDisk();

    float r1 = 0.5f;
    float r2 = 2.0f;

    float rho2 = DiskModelUtil::gasDensity(disk, r2);
    float rho1 = DiskModelUtil::gasDensity(disk, r1);
    float p1   = DiskModelUtil::pressure(disk, r1);
    float p2   = DiskModelUtil::pressure(disk, r2);

    REQUIRE(rho1 > rho2);
    REQUIRE(p1 > p2);
    REQUIRE(p1 / p2 > 1.0f); // Simple check that the ratio is positive
}

TEST_CASE("DiskModel produces realistic sound speed and Keplerian omega", "[disk][constants]")
{
    // Test derived properties (Sound speed and Keplerian frequency) against known physical constants
    auto disk = SciencePresets::solarNebula();

    const float cs = DiskModelUtil::soundSpeed(disk, 1.0f);
    // Sound speed in solar nebula at 1 AU should be around 500-1000 m/s
    REQUIRE(cs > 500.0f);
    REQUIRE(cs < 2000.0f);

    const float omega = DiskModelUtil::keplerianOmega(disk, 1.0f);
    // Omega at 1 AU should be ~1.99e-7 s⁻¹
    REQUIRE(omega > 1.5e-7f);
    REQUIRE(omega < 3.0e-7f);
}
