#pragma once
#include <cassert>
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

    // Read/Write access (Reference)
    [[nodiscard]] float &operator[](size_t i) noexcept
    {
        assert(i < 3 && "Vec3f index out of bounds");
        if(i == 0) return x;
        if(i == 1) return y;
        return z;
    }

    // Read-only access (Value is fine for scalars, or const ref)
    [[nodiscard]] float operator[](size_t i) const noexcept
    {
        assert(i < 3 && "Vec3f index out of bounds");
        if(i == 0) return x;
        if(i == 1) return y;
        return z;
    }


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

    // c++26 . . . . constexpr, noexcept FIXME when the day comes
    [[nodiscard]] float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr float lengthSq() const noexcept
    {
        return x * x + y * y + z * z;
    }

    // c++26 . . . . constexpr, noexcept
    [[nodiscard]] Vec3f normalize() const
    {
        const float len = length();
        if (len == static_cast<float>(0.0f))
            return *this;
        return {x / len, y / len, z / len};
    }

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


// Deterministic particle initialization
std::vector<Particle> genParticles(size_t n)
{
    std::vector<Particle> ps(n);
    for (size_t i = 0; i < n; ++i) {
        Particle p;
        p.id = i;
        p.position = { float(i) * 0.001f, float(i) * 0.002f, float(i) * 0.0005f };
        p.velocity = { 0.0f, 0.0f, 0.0f };
        p.acceleration = { 0.0f, 0.0f, 0.0f };
        p.mass = 1.0f;
        ps[i] = p;
    }
    return ps;
}



// A heavier deterministic force so integrator+adapter overhead is visible.
// This avoids random noise and avoids N^2, but creates meaningful CPU load.
void expensiveSpringForce(const std::vector<Particle>& ps, std::vector<Vec3f>& out_forces)
{
    const float k = 1.0f;
    const size_t n = ps.size();
    if (out_forces.size() != n)
        out_forces.resize(n);

    for (size_t i = 0; i < n; ++i) {
        const auto& p = ps[i];
        const float x = p.position.x;
        const float y = p.position.y;
        const float z = p.position.z;

        float fx = -k * x;
        float fy = -k * y;
        float fz = -k * z;

        // Burn a bit of predictable compute per particle.
        // (fast-math will make this cheaper but still stable)
        for (int j = 0; j < 8; ++j) {
            fx += std::sin(x + float(j)) * 0.01f;
            fy += std::cos(y + float(j)) * 0.01f;
            fz += std::sin(z + float(j)) * 0.01f;
        }

        out_forces[i] = { fx, fy, fz };
    }
}

// Inflatable bounce castle with blower
inline void springForce(const std::vector<Particle> &particles,
                        std::vector<Vec3f> &outForces)
{
    for (size_t i = 0; i < particles.size(); ++i){
        const auto &particle = particles[i];
        outForces[i] = particle.position * -1.0f;
    }
}

// Its wet here ... Damped spring: F = -k x - c v
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

// Calculate Total Energy (E = K + U) for simple spring (U = 0.5 * k * x^2)
inline double calculateTotalEnergy(const std::vector<Particle> &particles, float k_spring)
{
    double total_E = 0.0;
    for (const auto &p : particles) {
        // Kinetic Energy: K = 0.5 * m * v^2
        double v_sq = p.velocity.lengthSq();
        double K = 0.5 * p.mass * v_sq;

        // Potential Energy (Spring): U = 0.5 * k * x^2 (assuming origin is equilibrium)
        // Note: position is Vec3f, so x^2 is lengthSq()
        double dist_sq = p.position.lengthSq();
        double U = 0.5 * k_spring * dist_sq;

        total_E += (K + U);
    }
    return total_E;
}



} // namespace job::threads

// CHECKPOINT v1.2
