#pragma once

#include <real_type.h>
#include "data/vec3f.h"
#include "data/particle.h"
#include "data/disk.h"
#include "data/composition.h"

namespace job::science::data {

// Core physical force models for protoplanetary sorting
struct Forces final {
    static constexpr float G       = 6.67430e-11f;    // Gravitational constant
    static constexpr float M_sun   = 1.98847e30f;     // Solar mass (kg)
    static constexpr float c_light = 2.99792458e8f;   // Speed of light (m/s)
    static constexpr float Q_pr    = 1.0f;            // Radiation pressure coefficient

    // Gravitational acceleration toward the central star
    [[nodiscard]] static constexpr Vec3f gravity(const Particle &p, const DiskModel &disk) noexcept
    {
        const float r_m_sq = p.position.lengthSq(); // r² in meters
        if (r_m_sq <= 1e-9f)
            return kEmptyVec3F;

        const float M = disk.stellarMass * M_sun;
        const float mag_accel = G * M / r_m_sq;

        // Direction is opposite to position (inward)
        return p.position.normalize() * -mag_accel;
    }

    // Radiation pressure acceleration (isotropic)
    [[nodiscard]] static constexpr Vec3f radiationPressure(const Particle &p,
                                                           const DiskModel &disk,
                                                           const Composition &comp) noexcept
    {
        const float r_m_sq = p.position.lengthSq();
        if (r_m_sq <= float(1e-9) || p.mass <= float(0.0))
            return kEmptyVec3F;

        const float L_star = disk.stellarLuminosity * 3.828e26f;    // (W)
        const float area   = ParticleUtil::surfaceArea(p);                 // m²
        const float m      = p.mass;
        const float F_mag = (Q_pr * area * L_star) / (4.0f * core::piToTheFace * r_m_sq * c_light);
        const float a_mag = F_mag / m;
        const float final_accel = a_mag * disk.radiationPressureCoeff * comp.absorptivity;

        // outward
        return p.position.normalize() * final_accel;
    }

    // Photophoretic force (Rohatschek 1995 form, scaled radially)
    [[nodiscard]] static constexpr Vec3f photophoretic(const Particle &p, const DiskModel &disk, const Composition &comp) noexcept
    {
        const float r_m = p.position.length(); // r in meters
        if (r_m <= float(1e-9))
            return kEmptyVec3F;

        const float r_au  = DiskModelUtil::metersToAU(r_m);
        const float T     = DiskModelUtil::temperature(disk, r_au);
        const float rho   = DiskModelUtil::gasDensity(disk, r_au);

        // (requires detailed gas constants for full form)
        const float Kn = float(1e-2) / (rho * p.radius + float(1e-9));

        const float C_ph_base = (comp.absorptivity * T * T * Kn) / (p.radius * rho + float(1e-12));
        const float a_mag = C_ph_base * float(1e-18);

        //  outward
        return p.position.normalize() * a_mag;
    }

    [[nodiscard]] static constexpr Vec3f gasDrag(const Particle &p, const DiskModel &disk) noexcept
    {
        const float r_au    = DiskModelUtil::metersToAU(p.position.length());
        const float rho_gas = DiskModelUtil::gasDensity(disk, r_au);
        const float c_s     = DiskModelUtil::soundSpeed(disk, r_au);
        const float A       = ParticleUtil::surfaceArea(p);
        const float m       = p.mass;

        if (m <= float(0.0))
            return kEmptyVec3F;

        const float v_mag = p.velocity.length();
        const float a_drag_mag = (-rho_gas * c_s * A * v_mag) / (m + float(1e-9));

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
                float distSq = r.x * r.x + r.y * r.y + r.z * r.z;

                if (distSq <= 1e-12f)
                    continue;

                float invDist  = 1.0f / std::sqrt(distSq);
                float invDist3 = invDist * invDist * invDist;

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

            float r2 = dr.x * dr.x + dr.y * dr.y + dr.z * dr.z;
            if (r2 <= 1e-12f)
                continue;

            float invR  = 1.0f / std::sqrt(r2);
            float invR3 = invR * invR * invR;

            exact = exact + dr * (s.mass * invR3);
        }

        return exact;
    }


    // Inflatable bounce castle with blower
    static void springForce(const Particles &particles,
                            std::vector<Vec3f> &out_forces)
    {
        const float k = 1.0f;
        const size_t n = particles.size();
        out_forces.resize(n);

        for (size_t i = 0; i < n; ++i)
            out_forces[i] = particles[i].position * -k;
    }

    // Its wet here ... Damped spring: F = -k x - c v
    static void dampedSpringForce(const Particles &particles,
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

    // A heavier deterministic force so integrator+adapter overhead is visible.
    // This avoids random noise and avoids N^2, but creates meaningful CPU load.
    static void expensiveSpringForce(const std::vector<Particle>& ps, std::vector<Vec3f>& out_forces)
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


    // Calculate Total Energy (E = K + U) for simple spring (U = 0.5 * k * x^2)
    static double calculateTotalEnergy(const std::vector<Particle> &particles, float k_spring)
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

};

}
