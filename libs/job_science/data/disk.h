#pragma once

#include <cmath>
#include <cstdint>

#include <real_type.h>
namespace job::science::data {

enum class DiskZone : uint8_t {
    Inner_Disk = 0,
    Mid_Disk,
    Outer_Disk
};

#pragma pack(push, 1)
struct DiskModel final {
    core::real_t stellarMass{core::real_t(1.0)};               // Solar masses
    core::real_t stellarLuminosity{core::real_t(1.0)};         // Solar luminosities
    core::real_t stellarTemp{core::real_t(5778.0)};            // K, for solar-type star
    core::real_t innerRadius{core::real_t(0.05)};              // AU, sublimation limit (~1200 K)
    core::real_t outerRadius{core::real_t(100.0)};             // AU, cutoff for gas disk
    core::real_t scaleHeight{core::real_t(0.05)};              // AU at 1 AU, pressure scale height
    core::real_t T0{core::real_t(280.0)};                      // K, midplane temperature at 1 AU
    core::real_t rho0{core::real_t(1e-9f)};                    // kg/m³, gas density at 1 AU
    core::real_t pressure0{core::real_t(1e-3f)};               // Pa, midplane gas pressure at 1 AU
    core::real_t tempExponent{core::real_t(-0.5)};             // T ∝ r^tempExponent
    core::real_t densityExponent{core::real_t(-2.75)};         // ρ ∝ r^densityExponent
    core::real_t radiationPressureCoeff{core::real_t(1.0)};    // scaling factor for radiation effects
};
#pragma pack(pop)

struct DiskModelUtil final {
    [[nodiscard]] static constexpr bool isValid(const DiskModel &dm) noexcept
    {
        if (dm.innerRadius <= core::real_t(0) || dm.outerRadius <= dm.innerRadius)
            return false;

        if (dm.T0 <= core::real_t(0) || dm.rho0 <= core::real_t(0))
            return false;

        return true;
    }

    // AU ->  meters
    static constexpr core::real_t auToMeters(core::real_t au) noexcept
    {
        return au * core::real_t(1.495978707e11);
    }
    // meters -> AU
    static constexpr core::real_t metersToAU(core::real_t meters) noexcept
    {
        return meters / core::real_t(1.495978707e11);
    }

    // "Midplane" temperature @ distance r (AU)
    [[nodiscard]] static constexpr core::real_t temperature(const DiskModel &dm, core::real_t rAU) noexcept
    {
        return dm.T0 * std::pow(rAU, dm.tempExponent);
    }

    // Gas density at distance r (AU)
    [[nodiscard]] static constexpr core::real_t gasDensity(const DiskModel &dm, core::real_t rAU) noexcept
    {
        return dm.rho0 * std::pow(rAU, dm.densityExponent);
    }

    // Pressure (law scaled gas)
    [[nodiscard]] static constexpr core::real_t pressure(const DiskModel &dm, core::real_t rAU) noexcept
    {
        return dm.pressure0 * std::pow(rAU, dm.densityExponent + dm.tempExponent);
    }

    // Sound speed (isothermal)
    [[nodiscard]] static constexpr core::real_t soundSpeed(const DiskModel &dm, core::real_t rAU) noexcept
    {
        constexpr core::real_t k_B = core::real_t(1.380649e-23);   // Boltzmann constant
        constexpr core::real_t m_H = core::real_t(1.6735575e-27);  // Hydrogen mass
        constexpr core::real_t mu  = core::real_t(2.34);           // mean molecular weight (H₂ + He mix)
        const core::real_t T = DiskModelUtil::temperature(dm, rAU);
        return std::sqrt(k_B * T / (mu * m_H));
    }

    // radius -> orbital angular velocity (Keplerian)
    [[nodiscard]] static constexpr core::real_t keplerianOmega(const DiskModel &dm, core::real_t rAU) noexcept
    {
        constexpr core::real_t G      = core::real_t(6.67430e-11);
        constexpr core::real_t M_sun  = core::real_t(1.98847e30);
        const core::real_t r = auToMeters(rAU);
        const core::real_t M = dm.stellarMass * M_sun;
        return std::sqrt(G * M / (r * r * r));
    }
};

namespace SciencePresets {

constexpr DiskModel solarNebula() noexcept {
    return DiskModel{
        .stellarMass            = core::real_t(1.0),
        .stellarLuminosity      = core::real_t(1.0),
        .stellarTemp            = core::real_t(5778.0),
        .innerRadius            = core::real_t(0.05),
        .outerRadius            = core::real_t(100.0),
        .scaleHeight            = core::real_t(0.05),
        .T0                     = core::real_t(280.0),
        .rho0                   = core::real_t(1e-9),
        .pressure0              = core::real_t(1e-3),
        .tempExponent           = core::real_t(-0.5),
        .densityExponent        = core::real_t(-2.75),
        .radiationPressureCoeff = core::real_t(1.0)
    };
}

constexpr DiskModel tTauriDisk() noexcept {
    return DiskModel{
        .stellarMass            = core::real_t(0.7),
        .stellarLuminosity      = core::real_t(0.5),
        .stellarTemp            = core::real_t(4000.0),
        .innerRadius            = core::real_t(0.02),
        .outerRadius            = core::real_t(50.0),
        .scaleHeight            = core::real_t(0.04),
        .T0                     = core::real_t(400.0),
        .rho0                   = core::real_t(5e-9),
        .pressure0              = core::real_t(2e-3),
        .tempExponent           = core::real_t(-0.6),
        .densityExponent        = core::real_t(-2.5),
        .radiationPressureCoeff = core::real_t(0.8)
    };
}

constexpr DiskModel coldOuterDisk() noexcept {
    return DiskModel{
        .stellarMass                = core::real_t(1.0),
        .stellarLuminosity          = core::real_t(0.8),
        .stellarTemp                = core::real_t(5000.0),
        .innerRadius                = core::real_t(0.1),
        .outerRadius                = core::real_t(200.0),
        .scaleHeight                = core::real_t(0.07),
        .T0                         = core::real_t(180.0),
        .rho0                       = core::real_t(5e-10),
        .pressure0                  = core::real_t(5e-4),
        .tempExponent               = core::real_t(-0.5),
        .densityExponent            = core::real_t(-3.0),
        .radiationPressureCoeff     = core::real_t(0.9)
    };
}

}

}
