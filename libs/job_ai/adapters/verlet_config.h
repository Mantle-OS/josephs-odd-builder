#pragma once

#include <vec3f.h>
#include <vector>

namespace job::ai::adapters {
using Vec3 = job::science::data::Vec3f;
using Real = float;
struct Particle {
    Vec3 pos;
    Vec3 vel;
    Vec3 acc;
    Real mass;
};

struct VerletConfig {
    float dt = 0.01f;
    // Add specific force constants here if needed
};

inline void computeNbodyForces(std::vector<Particle> &particles)
{
    const size_t n = particles.size();
    // Small epsilon to prevent division by zero
    const float eps = 1e-5f;

    // Reset accelerations
    for (auto &p : particles) {
        p.acc = {0.0f, 0.0f, 0.0f};
    }

    // O(N^2) Direct Sum
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            auto& p1 = particles[i];
            auto& p2 = particles[j];

            // Vector r = p2 - p1
            Vec3 r;
            r.x = p2.pos.x - p1.pos.x;
            r.y = p2.pos.y - p1.pos.y;
            r.z = p2.pos.z - p1.pos.z;

            float distSq = r.x*r.x + r.y*r.y + r.z*r.z + eps;
            float dist = std::sqrt(distSq);
            float distCb = dist * distSq; // r^3

            // Force Magnitude F = (m1 * m2) / r^2
            // Acceleration contribution a = F / m
            // a1 += (m2 / r^3) * r
            // a2 -= (m1 / r^3) * r  (Newton's 3rd Law)

            // Assuming Gravitational attraction (Positive).
            // Make negative for electrostatic repulsion.
            float f1 = p2.mass / distCb;
            float f2 = p1.mass / distCb;

            p1.acc.x += r.x * f1;
            p1.acc.y += r.y * f1;
            p1.acc.z += r.z * f1;

            p2.acc.x -= r.x * f2;
            p2.acc.y -= r.y * f2;
            p2.acc.z -= r.z * f2;
        }
    }
}


} // namespace job::ai::adapters
