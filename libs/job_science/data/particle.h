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
    core::real_t        radius{core::real_t(0.0)};            // meters
    core::real_t        mass{core::real_t(0.0)};              // kg (derived)
    core::real_t        temperature{core::real_t(0.0)};       // Kalvin
    Composition         composition;                    // Composition holds density, heat capacity, emissivity, etc.
    DiskZone            zone{DiskZone::Inner_Disk};     // where in the disk the partical is.
    uint8_t             flags{0};                       // reserved bits for simulation states
};
#pragma pack(pop)

using Particles = std::vector<Particle>;

struct ParticleUtil final {
    [[nodiscard]] static constexpr bool isValid(const Particle &p) noexcept
    {
        return std::isfinite(p.radius) && p.radius > core::real_t(0.0);
    }

    [[nodiscard]] static constexpr core::real_t volume(const Particle &p) noexcept
    {
        return (core::real_t(4.0) / core::real_t(3.0)) * core::real_t(core::piToTheFace) * p.radius * p.radius * p.radius;
    }

    [[nodiscard]] static constexpr core::real_t surfaceArea(const Particle &p) noexcept
    {
        return core::real_t(4.0) * core::real_t(core::piToTheFace) * p.radius * p.radius;
    }

    [[nodiscard]] static constexpr core::real_t density(const Particle &p) noexcept
    {
        return p.composition.density;
    }

    // test particle
    [[nodiscard]] static constexpr Particle createTestParticle(core::real_t r_au, core::real_t radius, const Composition &comp) noexcept
    {
        Particle p{};
        // (X=r/sqrt(2), Y=r/sqrt(2))
        const core::real_t r_m = r_au * DiskModelUtil::auToMeters(core::real_t(1.0));
        const core::real_t r_m_comp = r_m / std::sqrt(core::real_t(2.0));
        p.position.x = r_m_comp;
        p.position.y = r_m_comp;

        p.radius = radius;
        p.composition = comp;

        // Calculated mass (V * rho)
        p.mass = ParticleUtil::volume(p) * p.composition.density;
        p.velocity = kEmptyVec3F;
        return p;
    }
    [[nodiscard]] static constexpr Particle createDragParticle(core::real_t r_au, core::real_t radius, core::real_t vy, const Composition &comp) noexcept
    {
        Particle p = ParticleUtil::createTestParticle(r_au, radius, comp);
        p.velocity.y = vy;
        return p;
    }

};

}
