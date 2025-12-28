#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <job_logger.h>

#include <data/disk.h>
#include <data/particle.h>
#include <data/zones.h>

using namespace job::science::data;

TEST_CASE("Zones validity and presets", "[zones][presets]")
{
    Zones solar = SciencePresets::solarSystemLike();
    Zones hot   = SciencePresets::compactHotDisk();
    Zones cold  = SciencePresets::coldExtendedDisk();

    REQUIRE(ZonesUtil::isValid(solar));
    REQUIRE(ZonesUtil::isValid(hot));
    REQUIRE(ZonesUtil::isValid(cold));

    REQUIRE(solar.innerBoundaryAU < solar.midBoundaryAU);
    REQUIRE(solar.midBoundaryAU   < solar.outerBoundaryAU);
}

TEST_CASE("Zones classifyByRadius basic thresholds", "[zones][radius]")
{
    Zones zones = SciencePresets::solarSystemLike();

    // Test inner region (0.3 AU boundary)
    REQUIRE(ZonesUtil::classifyByRadius(zones, 0.1f)  == DiskZone::Inner_Disk);
    // Test mid region (5.0 AU boundary)
    REQUIRE(ZonesUtil::classifyByRadius(zones, 1.0f)  == DiskZone::Mid_Disk);
    // Test outer region
    REQUIRE(ZonesUtil::classifyByRadius(zones, 50.0f) == DiskZone::Outer_Disk);
}

TEST_CASE("Zones classifyByTemperature boundaries", "[zones][temperature]")
{
    Zones zones = SciencePresets::solarSystemLike();

    // T_silicateSub = 1200 K -> Inner
    REQUIRE(ZonesUtil::classifyByTemperature(zones, 1300.0f) == DiskZone::Inner_Disk);
    // T_iceCondense = 170 K -> Mid
    REQUIRE(ZonesUtil::classifyByTemperature(zones, 200.0f)  == DiskZone::Mid_Disk);
    // T_COCondense = 30 K -> Outer
    REQUIRE(ZonesUtil::classifyByTemperature(zones, 20.0f)   == DiskZone::Outer_Disk);
}

TEST_CASE("Zones classify combined with disk model", "[zones][disk]")
{
    Zones zones = SciencePresets::solarSystemLike();
    DiskModel disk = SciencePresets::solarNebula();

    // Use explicit real_t casting for Particle position initialization
    Particle p = ParticleUtil::particleAtRadiusAU(0.05f);

    // Inner Disk (r=0.05 AU). T at this distance is > 1200K (T_silicateSub)
    p.position.x = 0.05f;
    DiskZone z1 = ZonesUtil::classify(zones, p, disk);
    REQUIRE(z1 == DiskZone::Inner_Disk);

    // Mid Disk (r=1.0 AU). T at this distance (~280K) is between T_silicateSub and T_iceCondense
    p = ParticleUtil::particleAtRadiusAU(1.0f);
    DiskZone z2 = ZonesUtil::classify(zones, p, disk);
    JOB_LOG_INFO("[ZonesUtil::classify] {}", static_cast<uint8_t>(z2));
    REQUIRE(z2 == DiskZone::Mid_Disk);

    // Outer Disk (r=50.0 AU). T at this distance is low, below T_iceCondense
    p = ParticleUtil::particleAtRadiusAU(50.0f);
    DiskZone z3 = ZonesUtil::classify(zones, p, disk);
    REQUIRE(z3 == DiskZone::Outer_Disk);
}

TEST_CASE("Zones nearBoundary detection", "[zones][boundary]")
{
    Zones zones = SciencePresets::solarSystemLike();
    DiskModel disk = SciencePresets::solarNebula();

    // Particle just beyond inner radial boundary (0.3 AU + 0.05 AU tolerance)
    Particle p = ParticleUtil::particleAtRadiusAU(zones.innerBoundaryAU + 0.05f);

    REQUIRE(ZonesUtil::nearBoundary(zones, p, disk));

    // Particle far from boundary should return false
    p = ParticleUtil::particleAtRadiusAU(10.0f);
    REQUIRE_FALSE(ZonesUtil::nearBoundary(zones, p, disk));
}

TEST_CASE("Zones snow line estimate", "[zones][snowline]")
{
    // Zones zones = SciencePresets::solarSystemLike();
    DiskModel disk = SciencePresets::solarNebula();

    const float snowAU = ZonesUtil::snowLineAU(disk);

    // Snow line should be physically realistic (e.g., 2.7 AU for a 1.0 Solar Mass star)
    REQUIRE(snowAU > 1.0f);
    REQUIRE(snowAU < disk.outerRadius);
}
