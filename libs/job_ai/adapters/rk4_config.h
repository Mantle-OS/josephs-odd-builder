#pragma once
#include <particle.h>
namespace job::ai::adapters {



struct Rk4Config {
    float   dt = 0.01f;                             // Time step
    int     steps = 1;                              // Number of RK4 steps per forward pass
    int     dim_mapping[7] = {0, 1, 2, 3, 4, 5, 6}; // Pos(3), Vel(3), Mass(1)
};




inline void computeNbodyForces(std::vector<science::data::Particle> &particles)
{
    const size_t n = particles.size();
    // Small epsilon to prevent division by zero
    const float eps = 1e-5f;

    // Reset accelerations
    for (auto &p : particles) {
        p.acceleration = {0.0f, 0.0f, 0.0f};
    }

    // O(N^2) Direct Sum
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            auto& p1 = particles[i];
            auto& p2 = particles[j];

            // Vector r = p2 - p1
            science::data::Vec3f r;
            r.x = p2.position.x - p1.position.x;
            r.y = p2.position.y - p1.position.y;
            r.z = p2.position.z - p1.position.z;

            float distSq = r.x*r.x + r.y*r.y + r.z*r.z + eps;
            float dist = std::sqrt(distSq);
            float distCb = dist * distSq; // r^3

            // Force Magnitude F = (m1 * m2) / r^2
            // Acceleration contribution a = F / m
            // a1 += (m2 / r^3) * r
            // a2 -= (m1 / r^3) * r  (Newton's 3rd Law)

            // Make negative for electrostatic repulsion.
            float f1 = p2.mass / distCb;
            float f2 = p1.mass / distCb;

            p1.acceleration.x += r.x * f1;
            p1.acceleration.y += r.y * f1;
            p1.acceleration.z += r.z * f1;

            p2.acceleration.x -= r.x * f2;
            p2.acceleration.y -= r.y * f2;
            p2.acceleration.z -= r.z * f2;
        }
    }
}





} // namespace job::ai::adapters
