#pragma once

#include <cstdint>

#include <real_type.h>

namespace job::science::data {

enum class MaterialType : uint8_t {
    Silicate = 0,               // Primarily Mg/Fe silicates (olivine, pyroxene); rocky grains
    Metallic,                   // Iron–nickel alloys or metallic compounds
    Icy,                        // Water/ammonia/methane ices, volatile-dominated grains
    Carbonaceous,               // Graphite, organics, complex hydrocarbons, soot-like material
    Sulfidic,                   // FeS, MgS, or similar sulfide-rich grains (Tag for a specific mix)
    Oxidized,                   // Fe₂O₃, Fe₃O₄, metal-oxide-dominated particles (Tag for a specific mix)
    Mixed                       // Composite / undifferentiated mixture
};

// Fractional elemental or phase composition of a particle
#pragma pack(push, 1)
struct Composition final {
    MaterialType type{MaterialType::Mixed};
    // Mass fractions (sum ≈ 1.0)
    core::real_t silicate{core::real_t(0.0)};      // Fractional silicate content (e.g. olivine, pyroxene, feldspar).
    core::real_t metal{core::real_t(0.0)};         // Fractional metallic content (e.g. Fe–Ni alloy).
    core::real_t ice{core::real_t(0.0)};           // Fractional volatile/ice content (H₂O, NH₃, CH₄ ices).
    core::real_t carbon{core::real_t(0.0)};        // Fractional carbonaceous content (soot, graphite, organics).
    // Derived or characteristic properties
    core::real_t density{core::real_t(0.0)};       // kg/m³
    core::real_t heatCapacity{core::real_t(0.0)};  // J/kg·K
    core::real_t emissivity{core::real_t(0.0)};    // dimensionless (0–1)
    core::real_t absorptivity{core::real_t(0.0)};  // dimensionless (0–1)

};
#pragma pack(pop)

struct CompositionUtil final {
    [[nodiscard]] static constexpr bool isValid(const Composition &c) noexcept
    {
        const core::real_t sum = totalFraction(c);

        if (sum <= core::real_t(0))
            return false;

        if (c.silicate < 0 || c.metal < 0 || c.ice < 0 || c.carbon < 0)
            return false;

        return sum > core::real_t(0.99) && sum < core::real_t(1.01);
    }
    [[nodiscard]] static constexpr core::real_t totalFraction(const Composition &c) noexcept
    {
        return c.silicate + c.metal + c.ice + c.carbon;
    }

    [[nodiscard]] static constexpr core::real_t reflectivity(const Composition &c) noexcept
    {
        return core::real_t(1.0) - c.absorptivity;
    }

    static constexpr void normalize(Composition &c) noexcept
    {
        const core::real_t sum = totalFraction(c);
        if (sum > static_cast<core::real_t>(0.0f)) {
            c.silicate /= sum;
            c.metal    /= sum;
            c.ice      /= sum;
            c.carbon   /= sum;
        }
    }

};

// Predefined compositions
namespace SciencePresets {

constexpr Composition rocky() noexcept {
    return Composition{
        .type       = MaterialType::Silicate,
        .silicate       = core::real_t(0.9),
        .metal          = core::real_t(0.1),
        .ice            = core::real_t(0.0),
        .carbon         = core::real_t(0.0),
        .density        = core::real_t(3300.0),
        .heatCapacity   = core::real_t(800.0),
        .emissivity     = core::real_t(0.9),
        .absorptivity   = core::real_t(0.7)
    };
}

constexpr Composition metallic() noexcept {
    return Composition{
        .type           = MaterialType::Metallic,
        .silicate       = core::real_t(0.1),
        .metal          = core::real_t(0.9),
        .ice            = core::real_t(0.0),
        .carbon         = core::real_t(0.0),
        .density        = core::real_t(7800.0),
        .heatCapacity   = core::real_t(450.0),
        .emissivity     = core::real_t(0.3),
        .absorptivity   = core::real_t(0.6)
    };
}

constexpr Composition icy() noexcept {
    return Composition{
        .type           = MaterialType::Icy,
        .silicate       = core::real_t(0.2),
        .metal          = core::real_t(0.0),
        .ice            = core::real_t(0.8),
        .carbon         = core::real_t(0.0),
        .density        = core::real_t(1200.0),
        .heatCapacity   = core::real_t(2100.0),
        .emissivity     = core::real_t(0.95),
        .absorptivity   = core::real_t(0.5)
    };
}

constexpr Composition carbonaceous() noexcept {
    return Composition{
        .type           = MaterialType::Carbonaceous,
        .silicate       = core::real_t(0.3),
        .metal          = core::real_t(0.1),
        .ice            = core::real_t(0.0),
        .carbon         = core::real_t(0.6),
        .density        = core::real_t(2000.0),
        .heatCapacity   = core::real_t(950.0),
        .emissivity     = core::real_t(0.8),
        .absorptivity   = core::real_t(0.7)
    };
}

constexpr Composition sulfidic() noexcept {
    return Composition{
        .type           = MaterialType::Sulfidic,
        .silicate       = core::real_t(0.4),
        .metal          = core::real_t(0.3),
        .ice            = core::real_t(0.0),
        .carbon         = core::real_t(0.3),
        .density        = core::real_t(4500.0),
        .heatCapacity   = core::real_t(650.0),
        .emissivity     = core::real_t(0.7),
        .absorptivity   = core::real_t(0.6)
    };
}
}
}
