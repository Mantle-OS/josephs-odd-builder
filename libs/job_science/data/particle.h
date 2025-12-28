#pragma once

#include <cstdint>
#include <cmath>
#include <vector>

#include <real_type.h>
#include "vec3f.h"
#include "disk.h"
#include "composition.h"

namespace job::science::data {

#pragma pack(push, 1)
struct Particle final {
    uint64_t            id{0};                          // Simple id of the partical
    Vec3f               position{};                     // meters, disc-centric coordinates
    Vec3f               velocity{};                     // m/s
    Vec3f               acceleration{};                 // current accumulated accel (photophoresis + gravity)
    float               radius{0.0f};                   // meters
    float               mass{0.0f};                     // kg (derived)
    float               temperature{0.0f};              // Kalvin
    Composition         composition{};                  // Composition holds density, heat capacity, emissivity, etc.
    DiskZone            zone{DiskZone::Inner_Disk};     // where in the disk the partical is.
    uint8_t             flags{0};                       // reserved bits for simulation states
};
#pragma pack(pop)

using Particles = std::vector<Particle>;

struct ParticleUtil final {
    [[nodiscard]] static constexpr bool isValid(const Particle& p) noexcept
    {
        return job::core::isSafeFinite(p.radius ) && p.radius > 0.0f;
    }

    [[nodiscard]] static constexpr float volume(const Particle &p) noexcept
    {
        return (4.0f / 3.0f) * core::piToTheFace * p.radius * p.radius * p.radius;
    }

    [[nodiscard]] static constexpr float surfaceArea(const Particle &p) noexcept
    {
        return 4.0f * core::piToTheFace * p.radius * p.radius;
    }

    [[nodiscard]] static constexpr float density(const Particle &p) noexcept
    {
        return p.composition.density;
    }

    // test particle
    [[nodiscard]] static constexpr Particle createTestParticle(float r_au, float radius, const Composition &comp = SciencePresets::rocky()) noexcept
    {
        Particle p{};
        // (X=r/sqrt(2), Y=r/sqrt(2))
        const float r_m = r_au * DiskModelUtil::auToMeters(1.0f);
        const float r_m_comp = r_m / std::sqrt(2.0f);
        p.position.x = r_m_comp;
        p.position.y = r_m_comp;

        p.radius = radius;
        p.composition = comp;

        // Calculated mass (V * rho)
        p.mass = ParticleUtil::volume(p) * p.composition.density;
        p.velocity = kEmptyVec3F;
        return p;
    }

    // Deterministic particle initialization
    static constexpr Particles genParticles(size_t n) noexcept
    {
        Particles ps(n);
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

    [[nodiscard]] static constexpr Particle createDragParticle(float r_au, float radius, float vy, const Composition &comp) noexcept
    {
        Particle p = ParticleUtil::createTestParticle(r_au, radius, comp);
        p.velocity.y = vy;
        return p;
    }

    [[nodiscard]] static constexpr Particle particleAtRadiusAU(float rAU) noexcept
    {
        Particle p{};
        p.position.x = DiskModelUtil::auToMeters(rAU);  // store meters
        p.position.y = 0.0f;
        p.position.z = 0.0f;
        return p;
    }


};

}
