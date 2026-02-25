#pragma once

#include <simd_provider.h>
#include <real_type.h>

#include "data/vec3f.h"
#include "data/particle.h"
#include "data/disk.h"
#include "data/composition.h"
namespace job::science::data {
using namespace job::simd;

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


    void calculateNonGravitationalForcesSimd(Particles &ps, const DiskModel &disk)
    {
        const size_t count = ps.size();
        if (count == 0) return;

        // Constants (Star-centric)
        const f32 invAU     = SIMD::set1(1.0f / 1.495978707e11f);
        const f32 rho0      = SIMD::set1(disk.rho0);
        const f32 T0        = SIMD::set1(disk.T0);
        const f32 densExp   = SIMD::set1(disk.densityExponent);
        const f32 tempExp   = SIMD::set1(disk.tempExponent);
        const f32 eps       = SIMD::set1(1e-12f);

        // Physical constants
        const f32 soundBase = SIMD::set1(std::sqrt(1.380649e-23f / (2.34f * 1.6735575e-27f)));
        const f32 knBase    = SIMD::set1(1e-2f);
        const f32 phBase    = SIMD::set1(1e-18f);

        const size_t simdLimit = count & ~7; // Only go up to the last multiple of 8

        for (size_t i = 0; i < simdLimit; i += 8) {
            // --- 1. GATHER ---
            // (Getting the data out of the messy structs and into tight registers)
            float tx[8], ty[8], tz[8], tvx[8], tvy[8], tvz[8], tm[8], tr[8], tabs[8];
            for(int j=0; j<8; ++j) {
                const auto &p = ps[i+j];
                tx[j] = p.position.x; ty[j] = p.position.y; tz[j] = p.position.z;
                tvx[j] = p.velocity.x; tvy[j] = p.velocity.y; tvz[j] = p.velocity.z;
                tm[j] = p.mass; tr[j] = p.radius; tabs[j] = p.composition.absorptivity;
            }

            f32 x = SIMD::pull(tx);  f32 y = SIMD::pull(ty);  f32 z = SIMD::pull(tz);
            f32 vx = SIMD::pull(tvx); f32 vy = SIMD::pull(tvy); f32 vz = SIMD::pull(tvz);
            f32 m = SIMD::pull(tm);  f32 r = SIMD::pull(tr);  f32 absorp = SIMD::pull(tabs);

            // --- 2. RADIUS & LOG-SPACE ---
            f32 r_m2  = SIMD::mul_plus(x, x, SIMD::mul_plus(y, y, SIMD::mul(z, z)));
            f32 inv_r = SIMD::rsqrt(r_m2);
            f32 r_m   = SIMD::mul(r_m2, inv_r); // r^2 * 1/r = r
            f32 r_au  = SIMD::mul(r_m, invAU);

            f32 log_r = SIMD::log(SIMD::max(r_au, eps));
            f32 temp  = SIMD::mul(T0, SIMD::exp(SIMD::mul(tempExp, log_r)));
            f32 rho   = SIMD::mul(rho0, SIMD::exp(SIMD::mul(densExp, log_r)));

            // --- 3. GAS DRAG ---
            f32 area  = SIMD::mul(SIMD::set1(4.0f * core::piToTheFace), SIMD::mul(r, r));
            f32 cs    = SIMD::mul(soundBase, SIMD::sqrt(temp));
            f32 v_mag = SIMD::sqrt(SIMD::mul_plus(vx, vx, SIMD::mul_plus(vy, vy, SIMD::mul(vz, vz))));

            f32 drag_mag = SIMD::div(
                SIMD::mul(SIMD::set1(-1.0f), SIMD::mul(rho, SIMD::mul(cs, SIMD::mul(area, v_mag)))),
                SIMD::add(m, eps)
                );
            f32 drag_k = SIMD::mul(drag_mag, SIMD::div(SIMD::set1(1.0f), SIMD::max(v_mag, eps)));

            // --- 4. PHOTOPHORETIC (The "Light Pusher") ---
            // Kn = 0.01 / (rho * radius + eps)
            f32 kn = SIMD::div(knBase, SIMD::mul_plus(rho, r, eps));
            // a_ph = (absorp * T^2 * Kn) / (r * rho + 1e-12) * phBase
            f32 ph_num = SIMD::mul(absorp, SIMD::mul(SIMD::mul(temp, temp), kn));
            f32 ph_den = SIMD::mul_plus(r, rho, SIMD::set1(1e-12f));
            f32 ph_mag = SIMD::mul(SIMD::div(ph_num, ph_den), phBase);
            f32 ph_k   = SIMD::mul(ph_mag, inv_r); // Direction is radial outward (pos * inv_r)

            // --- 5. SCATTER & ACCUMULATE ---
            for(int j=0; j<8; ++j) {
                auto &acc = ps[i+j].acceleration;
                // Drag (anti-parallel to velocity)
                acc.x += ((float*)&vx)[j] * ((float*)&drag_k)[j];
                acc.y += ((float*)&vy)[j] * ((float*)&drag_k)[j];
                acc.z += ((float*)&vz)[j] * ((float*)&drag_k)[j];

                // Photophoretic (radial outward)
                acc.x += ((float*)&x)[j] * ((float*)&ph_k)[j];
                acc.y += ((float*)&y)[j] * ((float*)&ph_k)[j];
                acc.z += ((float*)&z)[j] * ((float*)&ph_k)[j];
            }
        }

        // --- 6. HANDLE STRAGGLERS ---
        // Finishing the job for the leftovers.
        for (size_t i = simdLimit; i < count; ++i) {
            ps[i].acceleration += Forces::gasDrag(ps[i], disk);
            ps[i].acceleration += Forces::photophoretic(ps[i], disk, ps[i].composition);
        }
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


    static void calculateNetAccelerationSimd(Particles &ps, const DiskModel &disk)
    {
        const size_t count = ps.size();
        if (count == 0) return;

        // --- 0. CONSTANT SETUP ---
        // Pre-loading the "Stars of the Show" into registers
        const f32 GM      = SIMD::set1(disk.stellarMass * Forces::M_sun * Forces::G);
        const f32 L_star  = SIMD::set1(disk.stellarLuminosity * 3.828e26f);
        const f32 rad_k   = SIMD::set1(Forces::Q_pr / (4.0f * core::piToTheFace * Forces::c_light));
        const f32 invAU   = SIMD::set1(1.0f / 1.495978707e11f);
        const f32 rho0    = SIMD::set1(disk.rho0);
        const f32 T0      = SIMD::set1(disk.T0);
        const f32 densExp = SIMD::set1(disk.densityExponent);
        const f32 tempExp = SIMD::set1(disk.tempExponent);
        const f32 eps     = SIMD::set1(1e-12f);

        // Physics constants for gas
        const f32 soundBase = SIMD::set1(std::sqrt(1.380649e-23f / (2.34f * 1.6735575e-27f)));
        const f32 knBase    = SIMD::set1(1e-2f);
        const f32 phBase    = SIMD::set1(1e-18f);

        const size_t simdLimit = count & ~7;

        for (size_t i = 0; i < simdLimit; i += 8) {
            // --- 1. GATHER (The Necessary Evil) ---
            float tx[8], ty[8], tz[8], tvx[8], tvy[8], tvz[8], tm[8], tr[8], tabs[8];
            for(int j=0; j<8; ++j) {
                const auto &p = ps[i+j];
                tx[j] = p.position.x; ty[j] = p.position.y; tz[j] = p.position.z;
                tvx[j] = p.velocity.x; tvy[j] = p.velocity.y; tvz[j] = p.velocity.z;
                tm[j] = p.mass; tr[j] = p.radius; tabs[j] = p.composition.absorptivity;
            }

            const f32 x = SIMD::pull(tx);  const f32 y = SIMD::pull(ty);  const f32 z = SIMD::pull(tz);
            const f32 vx = SIMD::pull(tvx); const f32 vy = SIMD::pull(tvy); const f32 vz = SIMD::pull(tvz);
            const f32 m = SIMD::pull(tm);  const f32 r = SIMD::pull(tr);  const f32 absorp = SIMD::pull(tabs);

            // --- 2. DISTANCE MATH ---
            // Calculate r once, use everywhere.
            const f32 r2    = SIMD::mul_plus(x, x, SIMD::mul_plus(y, y, SIMD::mul(z, z)));
            const f32 mask  = SIMD::gt_ps(r2, eps); // The "Don't fall into the star" shield
            const f32 inv_r = SIMD::rsqrt(r2);
            const f32 r_m   = SIMD::mul(r2, inv_r);
            const f32 r_au  = SIMD::mul(r_m, invAU);

            // --- 3. LOG-DOMAIN PHYSICS (Power Laws) ---
            // Using your Estrin exp/log.
            // std::pow(r, exp) -> exp(exp * log(r))
            const f32 log_r = SIMD::log(SIMD::max(r_au, eps));
            const f32 temp  = SIMD::mul(T0, SIMD::exp(SIMD::mul(tempExp, log_r)));
            const f32 rho   = SIMD::mul(rho0, SIMD::exp(SIMD::mul(densExp, log_r)));

            // --- 4. GRAVITY & RADIATION (Radial Forces) ---
            // Gravity: a = -GM / r^2
            const f32 inv_r2 = SIMD::mul(inv_r, inv_r);
            const f32 grav_coeff = SIMD::mul(GM, SIMD::mul(inv_r2, inv_r)); // inward coefficient

            // Radiation: a = (L_star * area * abs * rad_k) / (r^2 * m)
            const f32 area = SIMD::mul(SIMD::set1(4.0f * core::piToTheFace), SIMD::mul(r, r));
            const f32 rad_coeff = SIMD::mul(SIMD::mul(L_star, SIMD::mul(area, absorp)),
                                            SIMD::mul(rad_k, SIMD::div(inv_r2, SIMD::add(m, eps))));

            // Photophoretic
            const f32 kn     = SIMD::div(knBase, SIMD::mul_plus(rho, r, eps));
            const f32 ph_mag = SIMD::mul(SIMD::div(SIMD::mul(absorp, SIMD::mul(SIMD::mul(temp, temp), kn)),
                                                   SIMD::mul_plus(r, rho, SIMD::set1(1e-12f))), phBase);

            // Combine radial terms: (Radiation + Photophoretic - Gravity) * (Position / r)
            const f32 radial_k = SIMD::sub(SIMD::mul(SIMD::add(rad_coeff, ph_mag), inv_r), grav_coeff);

            // --- 5. GAS DRAG (Velocity Force) ---
            const f32 cs    = SIMD::mul(soundBase, SIMD::sqrt(temp));
            const f32 v_mag = SIMD::sqrt(SIMD::mul_plus(vx, vx, SIMD::mul_plus(vy, vy, SIMD::mul(vz, vz))));
            const f32 drag_coeff = SIMD::div(SIMD::mul(SIMD::set1(-1.0f), SIMD::mul(rho, SIMD::mul(cs, SIMD::mul(area, v_mag)))),
                                             SIMD::add(m, eps));
            const f32 drag_k = SIMD::mul(drag_coeff, SIMD::div(SIMD::set1(1.0f), SIMD::max(v_mag, eps)));

            // --- 6. SCATTER AND FUSE ---
            for(int j=0; j<8; ++j) {
                auto &acc = ps[i+j].acceleration;
                // Apply masked radial forces
                acc.x = ((float*)&mask)[j] > 0 ? (((float*)&x)[j] * ((float*)&radial_k)[j]) : 0.0f;
                acc.y = ((float*)&mask)[j] > 0 ? (((float*)&y)[j] * ((float*)&radial_k)[j]) : 0.0f;
                acc.z = ((float*)&mask)[j] > 0 ? (((float*)&z)[j] * ((float*)&radial_k)[j]) : 0.0f;

                // Add drag (anti-parallel to velocity)
                acc.x += ((float*)&vx)[j] * ((float*)&drag_k)[j];
                acc.y += ((float*)&vy)[j] * ((float*)&drag_k)[j];
                acc.z += ((float*)&vz)[j] * ((float*)&drag_k)[j];
            }
        }

        // --- 7. THE STRAGGLER CLEANUP ---
        // Because 8 doesn't always go into N.
        for (size_t i = simdLimit; i < count; ++i)
            ps[i].acceleration = netAcceleration(ps[i], disk, ps[i].composition);
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



    void calculateStarForcesSimd(
        const float* posX, const float* posY, const float* posZ,
        const float* mass, const float* area, const float* absorptivity,
        float* accX, float* accY, float* accZ,
        size_t count, const DiskModel& disk)
    {
        // Pre-compute scalar constants into SIMD registers
        // GM: Because gravity is the law, not just a good idea.
        const f32 GM     = SIMD::set1(disk.stellarMass * Forces::M_sun * Forces::G);
        const f32 L_star = SIMD::set1(disk.stellarLuminosity * 3.828e26f);
        const f32 rad_k  = SIMD::set1(Forces::Q_pr / (4.0f * core::piToTheFace * Forces::c_light));
        const f32 eps    = SIMD::set1(1e-9f); // Don't divide by zero, the universe hates that.

        for (size_t i = 0; i < count; i += SIMD::width()) {
            // 1. Pull 8 particles worth of data
            const f32 x = SIMD::pull(&posX[i]);
            const f32 y = SIMD::pull(&posY[i]);
            const f32 z = SIMD::pull(&posZ[i]);
            const f32 m = SIMD::pull(&mass[i]);

            // 2. r^2 = x^2 + y^2 + z^2
            // Using FMA because we're high-class like that.
            const f32 r2 = SIMD::mul_plus(x, x, SIMD::mul_plus(y, y, SIMD::mul(z, z)));

            // Safety check: if r2 < eps, we're inside the star.
            // We mask it out so we don't divide by zero and summon a black hole.
            const f32 mask = SIMD::gt_ps(r2, eps);

            // 3. Reciprocal Square Root - The MVP of physics kernels.
            const f32 inv_r  = SIMD::rsqrt(r2);
            const f32 inv_r2 = SIMD::mul(inv_r, inv_r);

            // 4. Gravity Calculation (Inward)
            // a_grav = -GM / r^2 * (pos/r)  => -GM * pos * (inv_r^3)
            const f32 inv_r3      = SIMD::mul(inv_r2, inv_r);
            const f32 grav_coeff  = SIMD::mul(GM, inv_r3);

            // 5. Radiation Pressure (Outward)
            // a_rad = (L_star * area * absorptivity * rad_k) / (r^2 * mass)
            const f32 a   = SIMD::pull(&area[i]);
            const f32 abs = SIMD::pull(&absorptivity[i]);

            const f32 rad_num   = SIMD::mul(L_star, SIMD::mul(a, abs));
            const f32 rad_den   = SIMD::mul(r2, m);
            // Approximate 1/rad_den to keep the pipes moving.
            const f32 rad_coeff = SIMD::mul(rad_num, SIMD::mul(rad_k, SIMD::rsqrt(rad_den)));

            // 6. Fused Sum
            // total_accel = pos * (rad_coeff * inv_r - grav_coeff)
            // We multiply rad_coeff by inv_r because rad_coeff is just the magnitude.
            const f32 total_coeff = SIMD::sub(SIMD::mul(rad_coeff, inv_r), grav_coeff);

            // Store result (masked to handle the r < eps case)
            // If the mask is 0, the force is 0.
            SIMD::mov(&accX[i], SIMD::and_ps(mask, SIMD::mul(x, total_coeff)));
            SIMD::mov(&accY[i], SIMD::and_ps(mask, SIMD::mul(y, total_coeff)));
            SIMD::mov(&accZ[i], SIMD::and_ps(mask, SIMD::mul(z, total_coeff)));
        }
    }







};

}
