#pragma once

#include <real_type.h>
#include "data/vec3f.h"
#include "data/particle.h"
#include "data/disk.h"
#include "data/composition.h"

namespace job::science::data {

// Core physical force models for protoplanetary sorting
struct Forces final {
    static constexpr core::real_t G       = core::real_t(6.67430e-11);    // Gravitational constant
    static constexpr core::real_t M_sun   = core::real_t(1.98847e30);     // Solar mass (kg)
    static constexpr core::real_t c_light = core::real_t(2.99792458e8);   // Speed of light (m/s)
    static constexpr core::real_t Q_pr    = core::real_t(1.0);            // Radiation pressure coefficient

    // Gravitational acceleration toward the central star
    [[nodiscard]] static constexpr Vec3f gravity(const Particle &p, const DiskModel &disk) noexcept
    {
        const core::real_t r_m_sq = p.position.lengthSq(); // r² in meters
        if (r_m_sq <= core::real_t(1e-9))
            return kEmptyVec3F;

        const core::real_t M = disk.stellarMass * M_sun;
        const core::real_t mag_accel = G * M / r_m_sq;

        // Direction is opposite to position (inward)
        return p.position.normalize() * -mag_accel;
    }

    // Radiation pressure acceleration (isotropic)
    [[nodiscard]] static constexpr Vec3f radiationPressure(const Particle &p,
                                                           const DiskModel &disk,
                                                           const Composition &comp) noexcept
    {
        const core::real_t r_m_sq = p.position.lengthSq();
        if (r_m_sq <= core::real_t(1e-9) || p.mass <= core::real_t(0.0))
            return kEmptyVec3F;

        const core::real_t L_star = disk.stellarLuminosity * core::real_t(3.828e26);    // (W)
        const core::real_t area   = ParticleUtil::surfaceArea(p);                 // m²
        const core::real_t m      = p.mass;
        const core::real_t F_mag = (Q_pr * area * L_star) / (core::real_t(4.0) * core::real_t(core::piToTheFace) * r_m_sq * c_light);
        const core::real_t a_mag = F_mag / m;
        const core::real_t final_accel = a_mag * disk.radiationPressureCoeff * comp.absorptivity;

        // outward
        return p.position.normalize() * final_accel;
    }

    // Photophoretic force (Rohatschek 1995 form, scaled radially)
    [[nodiscard]] static constexpr Vec3f photophoretic(const Particle &p, const DiskModel &disk, const Composition &comp) noexcept
    {
        const core::real_t r_m = p.position.length(); // r in meters
        if (r_m <= core::real_t(1e-9))
            return kEmptyVec3F;

        const core::real_t r_au  = DiskModelUtil::metersToAU(r_m);
        const core::real_t T     = DiskModelUtil::temperature(disk, r_au);
        const core::real_t rho   = DiskModelUtil::gasDensity(disk, r_au);

        // (requires detailed gas constants for full form)
        const core::real_t Kn = core::real_t(1e-2) / (rho * p.radius + core::real_t(1e-9));

        const core::real_t C_ph_base = (comp.absorptivity * T * T * Kn) / (p.radius * rho + core::real_t(1e-12));
        const core::real_t a_mag = C_ph_base * core::real_t(1e-18);

        //  outward
        return p.position.normalize() * a_mag;
    }

    [[nodiscard]] static constexpr Vec3f gasDrag(const Particle &p, const DiskModel &disk) noexcept
    {
        const core::real_t r_au    = DiskModelUtil::metersToAU(p.position.length());
        const core::real_t rho_gas = DiskModelUtil::gasDensity(disk, r_au);
        const core::real_t c_s     = DiskModelUtil::soundSpeed(disk, r_au);
        const core::real_t A       = ParticleUtil::surfaceArea(p);
        const core::real_t m       = p.mass;

        if (m <= core::real_t(0.0))
            return kEmptyVec3F;

        const core::real_t v_mag = p.velocity.length();
        const core::real_t a_drag_mag = (-rho_gas * c_s * A * v_mag) / (m + core::real_t(1e-9));

        // Direction is opposite to velocity
        return p.velocity.normalize() * a_drag_mag;
    }

    // Net acceleration = sum of all major forces
    [[nodiscard]] static constexpr Vec3f netAcceleration(const Particle &p, const DiskModel &disk, const Composition &comp) noexcept
    {
        const Vec3f a_grav = gravity(p, disk);
        const Vec3f a_rad  = radiationPressure(p, disk, comp);
        const Vec3f a_ph   = photophoretic(p, disk, comp);
        const Vec3f a_drag = gasDrag(p, disk);

        // Uses Vec3f::operator+
        return a_grav + a_rad + a_ph + a_drag;
    }


    // Brute force N^2 to get the "Ground Truth"
    static void computeExactForces(Particles &particles) noexcept
    {
        const size_t n = particles.size();

        for (size_t i = 0; i < n; ++i) {
            particles[i].acceleration = kEmptyVec3F;

            for (size_t j = 0; j < n; ++j) {
                if (i == j)
                    continue;

                Vec3f r = particles[j].position - particles[i].position;
                core::real_t distSq = r.x * r.x + r.y * r.y + r.z * r.z;

                if (distSq <= core::real_t(1e-12))
                    continue;

                core::real_t invDist  = core::real_t(1) / std::sqrt(distSq);
                core::real_t invDist3 = invDist * invDist * invDist;

                Vec3f f = r * (particles[j].mass * invDist3); // Mass of *other*
                particles[i].acceleration = particles[i].acceleration + f;
            }
        }
    }

    [[nodiscard]] static Vec3f exactForceOnTarget(const Particles &sources,
                                                  const Particle &target) noexcept
    {
        Vec3f exact = kEmptyVec3F;

        for (const auto &s : sources) {
            Vec3f dr = s.position - target.position; // source - target

            core::real_t r2 = dr.x * dr.x + dr.y * dr.y + dr.z * dr.z;
            if (r2 <= core::real_t(1e-12))
                continue;

            core::real_t invR  = core::real_t(1) / std::sqrt(r2);
            core::real_t invR3 = invR * invR * invR;

            exact = exact + dr * (s.mass * invR3);
        }

        return exact;
    }


    // Inflatable bounce castle with blower
    inline void springForce(const Particles &particles,
                            std::vector<Vec3f> &out_forces)
    {
        const core::real_t k = core::real_t(1.0);
        const size_t n = particles.size();
        out_forces.resize(n);

        for (size_t i = 0; i < n; ++i)
            out_forces[i] = particles[i].position * -k;
    }

    // Its wet here ... Damped spring: F = -k x - c v
    inline void dampedSpringForce(const Particles &particles,
                                  std::vector<Vec3f> &out_forces)
    {
        const core::real_t k = core::real_t(1.0);
        const core::real_t c = core::real_t(0.5); // lightly squishy
        const size_t n = particles.size();
        out_forces.resize(n);

        for (size_t i = 0; i < n; ++i) {
            Vec3f spring = particles[i].position * -k;
            Vec3f drag   = particles[i].velocity * -c;
            out_forces[i] = spring + drag;
        }
    }

};

}
