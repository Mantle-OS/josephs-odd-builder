#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
namespace job::threads {


template <class S>
concept T_Scalar = std::floating_point<S>;
// Simple Hooke’s law spring: F = -k x


/*!
 * If you are reading this I have a project called protoplanetary_sort it has much better utils and
 * is even more low level than this ... I know hard to imagine anyways ..... This is only here for test.
 * if you are looking for a full lib on that. I suggested you checkout https://git.mantle-os.com/learning/protoplanetary_sort
*/

/////////////////////////////////////////////
// COMPOSITION
/////////////////////////////////////////////

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


/////////////////////////////////////////////
// DISK
/////////////////////////////////////////////
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
    float rho0{1e-9f};                     // kg/m³, gas density at 1 AU
    float pressure0{1e-3f};                // Pa, midplane gas pressure at 1 AU
    float tempExponent{-0.5f};             // T ∝ r^tempExponent
    float densityExponent{-2.75f};         // ρ ∝ r^densityExponent
    float radiationPressureCoeff{1.0f};    // scaling factor for radiation effects
};
#pragma pack(pop)


/////////////////////////////////////////////
// ZONES
/////////////////////////////////////////////
#pragma pack(push, 1)
struct Zones final {
    float innerBoundaryAU{0.3f};        // hot sublimation limit (~silicate line)
    float midBoundaryAU{5.0f};          // roughly beyond snow line
    float outerBoundaryAU{30.0f};       // cold region / Kuiper-like transition
    float T_silicateSub{1200.0f};       // K, dust sublimation
    float T_iceCondense{170.0f};        // K, water ice condensation
    float T_COCondense{30.0f};          // K, CO/N₂ ice condensation
};
#pragma pack(pop)

/////////////////////////////////////////////
// VEC3F
/////////////////////////////////////////////
#pragma pack(push, 1)
struct Vec3f final {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    [[nodiscard]] constexpr Vec3f operator+(const Vec3f &o) const noexcept
    {
        return {x + o.x, y + o.y, z + o.z};
    }

    [[nodiscard]] constexpr Vec3f operator-(const Vec3f &o) const noexcept
    {
        return {x - o.x, y - o.y, z - o.z};
    }

    [[nodiscard]] constexpr Vec3f operator*(float s) const noexcept
    {
        return {x * s, y * s, z * s};
    }

    [[nodiscard]] constexpr float length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr float lengthSq() const noexcept
    {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] constexpr Vec3f normalize() const noexcept
    {
        const float len = length();
        if (len == static_cast<float>(0.0f))
            return *this;
        return {x / len, y / len, z / len};
    }

    // [[nodiscard]] constexpr Vec3f vec_add(const Vec3f& a, const Vec3f& b)
    // {
    //     return a + b;
    // }

    // [[nodiscard]]  constexpr Vec3f vec_sub(const Vec3f& a, const Vec3f& b)
    // {
    //     return a - b;
    // }
    // [[nodiscard]]  constexpr Vec3f vec_mul(const Vec3f& v, float s)
    // {
    //     return v * s;
    // }
    // [[nodiscard]] constexpr float vec_len_sq(const Vec3f& v)
    // {
    //     return v.lengthSq();
    // }

};
#pragma pack(pop)

/////////////////////////////////////////////
// PARTICLE
/////////////////////////////////////////////
#pragma pack(push, 1)
struct Particle final {
    uint64_t id{0};                         // Simple id of the partical
    Vec3f position{};                       // meters, disc-centric coordinates
    Vec3f velocity{};                       // m/s
    Vec3f acceleration{};                   // current accumulated accel (photophoresis + gravity)
    float radius{0.0f};                     // meters
    float mass{0.0f};                       // kg (derived)
    float temperature{0.0f};                // Kalvin
    Composition composition{};              // Composition holds density, heat capacity, emissivity, etc.
    DiskZone zone{DiskZone::Inner_Disk};    // where in the disk the partical is.
    uint8_t flags{0};                       // reserved bits for simulation states
};
#pragma pack(pop)



inline void springForce(const std::vector<Particle> &particles,
                        std::vector<Vec3f> &out_forces)
{
    const float k = 1.0f;
    const size_t n = particles.size();
    out_forces.resize(n);

    for (size_t i = 0; i < n; ++i)
        out_forces[i] = particles[i].position * -k;
}

// Damped spring: F = -k x - c v
inline void dampedSpringForce(const std::vector<Particle> &particles,
                              std::vector<Vec3f> &out_forces)
{
    const float k = 1.0f;
    const float c = 0.5f; // lightly squishy
    const size_t n = particles.size();
    out_forces.resize(n);

    for (size_t i = 0; i < n; ++i) {
        Vec3f spring = particles[i].position * -k;
        Vec3f drag   = particles[i].velocity * -c;
        out_forces[i] = spring + drag;
    }
}






} // namespace job::threads
