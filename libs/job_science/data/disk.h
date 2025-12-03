#pragma once

#include <cmath>
#include <cstdint>

#include "data/real_type.h"

namespace job::science::data {

enum class DiskZone : uint8_t {
    Inner_Disk = 0,
    Mid_Disk,
    Outer_Disk
};

#pragma pack(push, 1)
struct DiskModel final {
    real_t stellarMass{real_t(1.0)};               // Solar masses
    real_t stellarLuminosity{real_t(1.0)};         // Solar luminosities
    real_t stellarTemp{real_t(5778.0)};            // K, for solar-type star
    real_t innerRadius{real_t(0.05)};              // AU, sublimation limit (~1200 K)
    real_t outerRadius{real_t(100.0)};             // AU, cutoff for gas disk
    real_t scaleHeight{real_t(0.05)};              // AU at 1 AU, pressure scale height
    real_t T0{real_t(280.0)};                      // K, midplane temperature at 1 AU
    real_t rho0{real_t(1e-9f)};                    // kg/m³, gas density at 1 AU
    real_t pressure0{real_t(1e-3f)};               // Pa, midplane gas pressure at 1 AU
    real_t tempExponent{real_t(-0.5)};             // T ∝ r^tempExponent
    real_t densityExponent{real_t(-2.75)};         // ρ ∝ r^densityExponent
    real_t radiationPressureCoeff{real_t(1.0)};    // scaling factor for radiation effects
};
#pragma pack(pop)

struct DiskModelUtil final {
    [[nodiscard]] static constexpr bool isValid(const DiskModel &dm) noexcept
    {
        if (dm.innerRadius <= real_t(0) || dm.outerRadius <= dm.innerRadius)
            return false;

        if (dm.T0 <= real_t(0) || dm.rho0 <= real_t(0))
            return false;

        return true;
    }

    // AU ->  meters
    static constexpr real_t auToMeters(real_t au) noexcept
    {
        return au * real_t(1.495978707e11);
    }
    // meters -> AU
    static constexpr real_t metersToAU(real_t meters) noexcept
    {
        return meters / real_t(1.495978707e11);
    }

    // "Midplane" temperature @ distance r (AU)
    [[nodiscard]] static constexpr real_t temperature(const DiskModel &dm, real_t rAU) noexcept
    {
        return dm.T0 * std::pow(rAU, dm.tempExponent);
    }

    // Gas density at distance r (AU)
    [[nodiscard]] static constexpr real_t gasDensity(const DiskModel &dm, real_t rAU) noexcept
    {
        return dm.rho0 * std::pow(rAU, dm.densityExponent);
    }

    // Pressure (law scaled gas)
    [[nodiscard]] static constexpr real_t pressure(const DiskModel &dm, real_t rAU) noexcept
    {
        return dm.pressure0 * std::pow(rAU, dm.densityExponent + dm.tempExponent);
    }

    // Sound speed (isothermal)
    [[nodiscard]] static constexpr real_t soundSpeed(const DiskModel &dm, real_t rAU) noexcept
    {
        constexpr real_t k_B = real_t(1.380649e-23);   // Boltzmann constant
        constexpr real_t m_H = real_t(1.6735575e-27);  // Hydrogen mass
        constexpr real_t mu  = real_t(2.34);           // mean molecular weight (H₂ + He mix)
        const real_t T = DiskModelUtil::temperature(dm, rAU);
        return std::sqrt(k_B * T / (mu * m_H));
    }

    // radius -> orbital angular velocity (Keplerian)
    [[nodiscard]] static constexpr real_t keplerianOmega(const DiskModel &dm, real_t rAU) noexcept
    {
        constexpr real_t G      = real_t(6.67430e-11);
        constexpr real_t M_sun  = real_t(1.98847e30);
        const real_t r = auToMeters(rAU);
        const real_t M = dm.stellarMass * M_sun;
        return std::sqrt(G * M / (r * r * r));
    }
};

namespace SciencePresets {

constexpr DiskModel solarNebula() noexcept {
    return DiskModel{
        .stellarMass            = real_t(1.0),
        .stellarLuminosity      = real_t(1.0),
        .stellarTemp            = real_t(5778.0),
        .innerRadius            = real_t(0.05),
        .outerRadius            = real_t(100.0),
        .scaleHeight            = real_t(0.05),
        .T0                     = real_t(280.0),
        .rho0                   = real_t(1e-9),
        .pressure0              = real_t(1e-3),
        .tempExponent           = real_t(-0.5),
        .densityExponent        = real_t(-2.75),
        .radiationPressureCoeff = real_t(1.0)
    };
}

constexpr DiskModel tTauriDisk() noexcept {
    return DiskModel{
        .stellarMass            = real_t(0.7),
        .stellarLuminosity      = real_t(0.5),
        .stellarTemp            = real_t(4000.0),
        .innerRadius            = real_t(0.02),
        .outerRadius            = real_t(50.0),
        .scaleHeight            = real_t(0.04),
        .T0                     = real_t(400.0),
        .rho0                   = real_t(5e-9),
        .pressure0              = real_t(2e-3),
        .tempExponent           = real_t(-0.6),
        .densityExponent        = real_t(-2.5),
        .radiationPressureCoeff = real_t(0.8)
    };
}

constexpr DiskModel coldOuterDisk() noexcept {
    return DiskModel{
        .stellarMass                = real_t(1.0),
        .stellarLuminosity          = real_t(0.8),
        .stellarTemp                = real_t(5000.0),
        .innerRadius                = real_t(0.1),
        .outerRadius                = real_t(200.0),
        .scaleHeight                = real_t(0.07),
        .T0                         = real_t(180.0),
        .rho0                       = real_t(5e-10),
        .pressure0                  = real_t(5e-4),
        .tempExponent               = real_t(-0.5),
        .densityExponent            = real_t(-3.0),
        .radiationPressureCoeff     = real_t(0.9)
    };
}

}

}
