#pragma once

#include <cmath>

#include <real_type.h>

#include "data/disk.h"
#include "data/particle.h"

namespace job::science::data {

#pragma pack(push, 1)
struct Zones final {
    core::real_t innerBoundaryAU{core::real_t(0.3)};       // hot sublimation limit (~silicate line)
    core::real_t midBoundaryAU{core::real_t(5.0)};         // roughly beyond snow line
    core::real_t outerBoundaryAU{core::real_t(30.0)};      // cold region / Kuiper-like transition
    core::real_t T_silicateSub{core::real_t(1200.0)};      // K, dust sublimation
    core::real_t T_iceCondense{core::real_t(170.0)};       // K, water ice condensation
    core::real_t T_COCondense{core::real_t(30.0)};         // K, CO/N₂ ice condensation
};
#pragma pack(pop)

struct ZonesUtil final {
    [[nodiscard]] static constexpr bool isValid(const Zones &in) noexcept
    {
        return in.innerBoundaryAU > core::real_t(0.0) &&
               in.midBoundaryAU > in.innerBoundaryAU &&
               in.outerBoundaryAU > in.midBoundaryAU;
    }

    // AU
    [[nodiscard]] static constexpr DiskZone classifyByRadius(const Zones &in, core::real_t rAU) noexcept
    {
        if (rAU < in.innerBoundaryAU)
            return DiskZone::Inner_Disk;

        if (rAU < in.midBoundaryAU)
            return DiskZone::Mid_Disk;

        return DiskZone::Outer_Disk;
    }

    [[nodiscard]] static constexpr DiskZone classifyByTemperature(const Zones &in, core::real_t temperatureK) noexcept
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
        const core::real_t rMeters = p.position.x;
        const core::real_t rAU     = DiskModelUtil::metersToAU(rMeters);
        const core::real_t T = DiskModelUtil::temperature(disk, rAU);
        return classifyByTemperature(in, T);
    }

    // snow-line(T = 170 K)
    [[nodiscard]] static constexpr core::real_t snowLineAU(const DiskModel &disk) noexcept
    {
        return std::pow(core::real_t(170.0) / disk.T0, core::real_t(1.0) / disk.tempExponent);
    }

    [[nodiscard]] static constexpr bool nearBoundary(const Zones &in,
                                                     const Particle &p,
                                                     const DiskModel &disk,
                                                     core::real_t toleranceAU = core::real_t(0.1),
                                                     core::real_t toleranceK  = core::real_t(25.0)) noexcept
    {
        const core::real_t rMeters = p.position.x;         // or length()
        const core::real_t rAU     = DiskModelUtil::metersToAU(rMeters);
        const core::real_t T       = DiskModelUtil::temperature(disk, rAU);

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
        .innerBoundaryAU    = core::real_t(0.3),
        .midBoundaryAU      = core::real_t(5.0),
        .outerBoundaryAU    = core::real_t(30.0),
        .T_silicateSub      = core::real_t(1200.0),
        .T_iceCondense      = core::real_t(170.0),
        .T_COCondense       = core::real_t(30.0)
    };
}

constexpr Zones compactHotDisk() noexcept {
    return Zones{
        .innerBoundaryAU    = core::real_t(0.1),
        .midBoundaryAU      = core::real_t(2.0),
        .outerBoundaryAU    = core::real_t(10.0),
        .T_silicateSub      = core::real_t(1300.0),
        .T_iceCondense      = core::real_t(200.0),
        .T_COCondense       = core::real_t(40.0)
    };
}

constexpr Zones coldExtendedDisk() noexcept {
    return Zones{
        .innerBoundaryAU    = core::real_t(0.5),
        .midBoundaryAU      = core::real_t(10.0),
        .outerBoundaryAU    = core::real_t(100.0),
        .T_silicateSub      = core::real_t(1000.0),
        .T_iceCondense      = core::real_t(150.0),
        .T_COCondense       = core::real_t(25.0)
    };
}

}
}
