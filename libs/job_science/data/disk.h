#pragma once

#include <cmath>
#include <cstdint>

namespace job::science::data {

enum class DiskZone : uint8_t {
    Inner_Disk = 0,
    Mid_Disk,
    Outer_Disk
};

#pragma pack(push, 1)
struct DiskModel final {
    float stellarMass{1.0f};               // Solar masses
    float stellarLuminosity{1.0f};         // Solar luminosities
    float stellarTemp{5778.0f};            // K, for solar-type star
    float innerRadius{0.05f};              // AU, sublimation limit (~1200 K)
    float outerRadius{100.0f};             // AU, cutoff for gas disk
    float scaleHeight{0.05f};              // AU at 1 AU, pressure scale height
    float T0{280.0f};                      // K, midplane temperature at 1 AU
    float rho0{1e-9f};                    // kg/m³, gas density at 1 AU
    float pressure0{1e-3f};               // Pa, midplane gas pressure at 1 AU
    float tempExponent{-0.5f};             // T ∝ r^tempExponent
    float densityExponent{-2.75f};         // ρ ∝ r^densityExponent
    float radiationPressureCoeff{   1.0f};    // scaling factor for radiation effects
};
#pragma pack(pop)

struct DiskModelUtil final {
    [[nodiscard]] static constexpr bool isValid(const DiskModel &dm) noexcept
    {
        if (dm.innerRadius <= 0.0f || dm.outerRadius <= dm.innerRadius)
            return false;

        if (dm.T0 <= 0.0f || dm.rho0 <= 0.0f)
            return false;

        return true;
    }

    // AU ->  meters
    static constexpr float auToMeters(float au) noexcept
    {
        return au * 1.495978707e11f;
    }
    // meters -> AU
    static constexpr float metersToAU(float meters) noexcept
    {
        return meters / 1.495978707e11f;
    }

    // "Midplane" temperature @ distance r (AU)
    [[nodiscard]] static constexpr float temperature(const DiskModel &dm, float rAU) noexcept
    {
        return dm.T0 * std::pow(rAU, dm.tempExponent);
    }

    // Gas density at distance r (AU)
    [[nodiscard]] static constexpr float gasDensity(const DiskModel &dm, float rAU) noexcept
    {
        return dm.rho0 * std::pow(rAU, dm.densityExponent);
    }

    // Pressure (law scaled gas)
    [[nodiscard]] static constexpr float pressure(const DiskModel &dm, float rAU) noexcept
    {
        return dm.pressure0 * std::pow(rAU, dm.densityExponent + dm.tempExponent);
    }

    // Sound speed (isothermal)
    [[nodiscard]] static constexpr float soundSpeed(const DiskModel &dm, float rAU) noexcept
    {
        constexpr float k_B = 1.380649e-23f;   // Boltzmann constant
        constexpr float m_H = 1.6735575e-27f;  // Hydrogen mass
        constexpr float mu  = 2.34f;           // mean molecular weight (H₂ + He mix)
        const float T = DiskModelUtil::temperature(dm, rAU);
        return std::sqrt(k_B * T / (mu * m_H));
    }

    // radius -> orbital angular velocity (Keplerian)
    [[nodiscard]] static constexpr float keplerianOmega(const DiskModel &dm, float rAU) noexcept
    {
        constexpr float G      = 6.67430e-11f;
        constexpr float M_sun  = 1.98847e30f;
        const float r = auToMeters(rAU);
        const float M = dm.stellarMass * M_sun;
        return std::sqrt(G * M / (r * r * r));
    }
};

namespace SciencePresets {

constexpr DiskModel solarNebula() noexcept {
    return DiskModel{
        .stellarMass            = 1.0f,
        .stellarLuminosity      = 1.0f,
        .stellarTemp            = 5778.0f,
        .innerRadius            = 0.05f,
        .outerRadius            = 100.0f,
        .scaleHeight            = 0.05f,
        .T0                     = 280.0f,
        .rho0                   = 1e-9f,
        .pressure0              = 1e-3f,
        .tempExponent           = -0.5f,
        .densityExponent        = -2.75f,
        .radiationPressureCoeff = 1.0f
    };
}

constexpr DiskModel tTauriDisk() noexcept {
    return DiskModel{
        .stellarMass            = 0.7f,
        .stellarLuminosity      = 0.5f,
        .stellarTemp            = 4000.0f,
        .innerRadius            = 0.02f,
        .outerRadius            = 50.0f,
        .scaleHeight            = 0.04f,
        .T0                     = 400.0f,
        .rho0                   = 5e-9f,
        .pressure0              = 2e-3f,
        .tempExponent           = -0.6f,
        .densityExponent        = -2.5f,
        .radiationPressureCoeff = 0.8f
    };
}

constexpr DiskModel coldOuterDisk() noexcept {
    return DiskModel{
        .stellarMass                = 1.0f,
        .stellarLuminosity          = 0.8f,
        .stellarTemp                = 5000.0f,
        .innerRadius                = 0.1f,
        .outerRadius                = 200.0f,
        .scaleHeight                = 0.07f,
        .T0                         = 180.0f,
        .rho0                       = 5e-10f,
        .pressure0                  = 5e-4f,
        .tempExponent               = -0.5f,
        .densityExponent            = -3.0f,
        .radiationPressureCoeff     = 0.9f
    };
}

}

}
