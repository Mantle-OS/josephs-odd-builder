#pragma once

#include <cmath>

#include "data/disk.h"
#include "data/particle.h"

namespace job::science::data {

#pragma pack(push, 1)
struct Zones final {
    float innerBoundaryAU{0.3f};       // hot sublimation limit (~silicate line)
    float midBoundaryAU{5.0f};         // roughly beyond snow line
    float outerBoundaryAU{30.0f};      // cold region / Kuiper-like transition
    float T_silicateSub{1200.0f};      // K, dust sublimation
    float T_iceCondense{170.0f};       // K, water ice condensation
    float T_COCondense{30.0f};         // K, CO/N₂ ice condensation
};
#pragma pack(pop)

struct ZonesUtil final {
    [[nodiscard]] static constexpr bool isValid(const Zones &in) noexcept
    {
        return in.innerBoundaryAU > 0.0f &&
               in.midBoundaryAU > in.innerBoundaryAU &&
               in.outerBoundaryAU > in.midBoundaryAU;
    }

    // AU
    [[nodiscard]] static constexpr DiskZone classifyByRadius(const Zones &in, float rAU) noexcept
    {
        if (rAU < in.innerBoundaryAU)
            return DiskZone::Inner_Disk;

        if (rAU < in.midBoundaryAU)
            return DiskZone::Mid_Disk;

        return DiskZone::Outer_Disk;
    }

    [[nodiscard]] static constexpr DiskZone classifyByTemperature(const Zones &in, float temperatureK) noexcept
    {
        if (temperatureK >= in.T_silicateSub)
            return DiskZone::Inner_Disk;

        if (temperatureK >= in.T_iceCondense)
            return DiskZone::Mid_Disk;

        return DiskZone::Outer_Disk;
    }

    [[nodiscard]] static constexpr DiskZone classify(const Zones &in,
                                                     const Particle &p,
                                                     const DiskModel &disk) noexcept
    {
        // position is in meters; convert to AU
        const float rMeters = p.position.x;
        const float rAU = DiskModelUtil::metersToAU(rMeters);
        const float T = DiskModelUtil::temperature(disk, rAU);
        return classifyByTemperature(in, T);
    }

    // snow-line(T = 170 K)
    [[nodiscard]] static constexpr float snowLineAU(const DiskModel &disk) noexcept
    {
        return std::pow(170.0f / disk.T0, 1.0f / disk.tempExponent);
    }

    [[nodiscard]] static constexpr bool nearBoundary(const Zones &in,
                                                     const Particle &p,
                                                     const DiskModel &disk,
                                                     float toleranceAU = 0.1f,
                                                     float toleranceK  = 25.0f) noexcept
    {
        const float rMeters = p.position.x;         // or length()
        const float rAU = DiskModelUtil::metersToAU(rMeters);
        const float T = DiskModelUtil::temperature(disk, rAU);

        const bool nearRadial =
            (std::fabs(rAU - in.innerBoundaryAU) < toleranceAU) ||
            (std::fabs(rAU - in.midBoundaryAU)   < toleranceAU) ||
            (std::fabs(rAU - in.outerBoundaryAU) < toleranceAU);

        const bool nearThermal =
            (std::fabs(T - in.T_silicateSub) < toleranceK)   ||
            (std::fabs(T - in.T_iceCondense) < toleranceK)   ||
            (std::fabs(T - in.T_COCondense)  < toleranceK);

        return nearRadial || nearThermal;
    }
};

namespace SciencePresets {

constexpr Zones solarSystemLike() noexcept {
    return Zones{
        .innerBoundaryAU    = 0.3f,
        .midBoundaryAU      = 5.0f,
        .outerBoundaryAU    = 30.0f,
        .T_silicateSub      = 1200.0f,
        .T_iceCondense      = 170.0f,
        .T_COCondense       = 30.0f
    };
}

constexpr Zones compactHotDisk() noexcept {
    return Zones{
        .innerBoundaryAU    = 0.1f,
        .midBoundaryAU      = 2.0f,
        .outerBoundaryAU    = 10.0f,
        .T_silicateSub      = 1300.0f,
        .T_iceCondense      = 200.0f,
        .T_COCondense       = 40.0f
    };
}

constexpr Zones coldExtendedDisk() noexcept {
    return Zones{
        .innerBoundaryAU    = 0.5f,
        .midBoundaryAU      = 10.0f,
        .outerBoundaryAU    = 100.0f,
        .T_silicateSub      = 1000.0f,
        .T_iceCondense      = 150.0f,
        .T_COCondense       = 25.0f
    };
}

} // SciencePresets

}
