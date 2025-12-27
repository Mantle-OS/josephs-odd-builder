#pragma once

#include <cstdint>

namespace job::science::data {

enum class MaterialType : uint8_t {
    Silicate        = 0,    // Primarily Mg/Fe silicates (olivine, pyroxene); rocky grains
    Metallic        = 1,    // Iron–nickel alloys or metallic compounds
    Icy             = 2,    // Water/ammonia/methane ices, volatile-dominated grains
    Carbonaceous    = 3,    // Graphite, organics, complex hydrocarbons, soot-like material
    Sulfidic        = 4,    // FeS, MgS, or similar sulfide-rich grains (Tag for a specific mix)
    Oxidized        = 5,    // Fe₂O₃, Fe₃O₄, metal-oxide-dominated particles (Tag for a specific mix)
    Mixed           = 7     // Composite / undifferentiated mixture
};

// Fractional elemental or phase composition of a particle
#pragma pack(push, 1)
struct Composition final {
    MaterialType type{MaterialType::Mixed};
    // Mass fractions (sum ≈ 1.0)
    float silicate{0.0f};      // Fractional silicate content (e.g. olivine, pyroxene, feldspar).
    float metal{0.0f};         // Fractional metallic content (e.g. Fe–Ni alloy).
    float ice{0.0f};           // Fractional volatile/ice content (H₂O, NH₃, CH₄ ices).
    float carbon{0.0f};        // Fractional carbonaceous content (soot, graphite, organics).
    // Derived or characteristic properties
    float density{0.0f};       // kg/m³
    float heatCapacity{0.0f};  // J/kg·K
    float emissivity{0.0f};    // dimensionless (0–1)
    float absorptivity{0.0f};  // dimensionless (0–1)

};
#pragma pack(pop)

struct CompositionUtil final {
    [[nodiscard]] static constexpr bool isValid(const Composition &c) noexcept
    {
        const float sum = totalFraction(c);

        if (sum <= 0.0f)
            return false;

        if (c.silicate < 0 || c.metal < 0 || c.ice < 0 || c.carbon < 0)
            return false;

        return sum > 0.99f && sum < 1.01f;
    }
    [[nodiscard]] static constexpr float totalFraction(const Composition &c) noexcept
    {
        return c.silicate + c.metal + c.ice + c.carbon;
    }

    [[nodiscard]] static constexpr float reflectivity(const Composition &c) noexcept
    {
        return 1.0f - c.absorptivity;
    }

    static constexpr void normalize(Composition &c) noexcept
    {
        const float sum = totalFraction(c);
        if (sum > 0.0f) {
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
        .silicate       = 0.9f,
        .metal          = 0.1f,
        .ice            = 0.0f,
        .carbon         = 0.0f,
        .density        = 3300.0f,
        .heatCapacity   = 800.0f,
        .emissivity     = 0.9f,
        .absorptivity   = 0.7f
    };
}

constexpr Composition metallic() noexcept {
    return Composition{
        .type           = MaterialType::Metallic,
        .silicate       = 0.1f,
        .metal          = 0.9f,
        .ice            = 0.0f,
        .carbon         = 0.0f,
        .density        = 7800.0f,
        .heatCapacity   = 450.0f,
        .emissivity     = 0.3f,
        .absorptivity   = 0.6f
    };
}

constexpr Composition icy() noexcept {
    return Composition{
        .type           = MaterialType::Icy,
        .silicate       = 0.2f,
        .metal          = 0.0f,
        .ice            = 0.8f,
        .carbon         = 0.0f,
        .density        = 1200.0f,
        .heatCapacity   = 2100.0f,
        .emissivity     = 0.95f,
        .absorptivity   = 0.5f
    };
}

constexpr Composition carbonaceous() noexcept {
    return Composition{
        .type           = MaterialType::Carbonaceous,
        .silicate       = 0.3f,
        .metal          = 0.1f,
        .ice            = 0.0f,
        .carbon         = 0.6f,
        .density        = 2000.0f,
        .heatCapacity   = 950.0f,
        .emissivity     = 0.8f,
        .absorptivity   = 0.7f
    };
}

constexpr Composition sulfidic() noexcept {
    return Composition{
        .type           = MaterialType::Sulfidic,
        .silicate       = 0.4f,
        .metal          = 0.3f,
        .ice            = 0.0f,
        .carbon         = 0.3f,
        .density        = 4500.0f,
        .heatCapacity   = 650.0f,
        .emissivity     = 0.7f,
        .absorptivity   = 0.6f
    };
}
}
}
