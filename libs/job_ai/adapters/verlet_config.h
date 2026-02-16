#pragma once
#include <cstdint>
#include <vector>
#include <data/particle.h>
#include <data/vec3f.h>
namespace job::ai::adapters {
using job::science::data::Vec3f;
using job::science::data::Particle;

inline constexpr uint8_t kVV_Velocity = 6;
inline constexpr uint8_t kVV_Mass = 7;

struct VerletConfig {
    float dt = 0.01f;
    // Add specific force constants here if needed
};

struct PhysicsContext {
    Particle        *particles;
    size_t          count;
    const float     *valueIn;  // Read-only V from previous step
    float           *valueOut; // Accumulator for V in this step
    int             valueDim;  // How many floats is V? (D - 7)
    int             stride;    // Total D (stride between particles)
};

inline void computeForcesAndMixing(PhysicsContext& ctx)
{
    const float epsilon         = 0.1f;
    const float G_coupling      = 1.0f;

    for (size_t i = 0; i < ctx.count; ++i) {
        // 1. Reset Physics (Acceleration always starts at 0 for the new frame)
        ctx.particles[i].acceleration = Vec3f{0,0,0};

        // 2. AI FIX: RESIDUAL CONNECTION
        // Initialize the output buffer with the particle's EXISTING thoughts (selfV).
        // This prevents "Amnesia" - if no one talks to me, I remember what I knew before.
        float *outV = ctx.valueOut + (i * ctx.stride) + 7;
        const float *selfV = ctx.valueIn + (i * ctx.stride) + 7;

        if (ctx.valueDim > 0) {
            for(int k=0; k<ctx.valueDim; ++k) {
                outV[k] = selfV[k];
            }
        }

        for (size_t j = 0; j < ctx.count; ++j) {
            if (i == j) continue;

            auto r = ctx.particles[j].position - ctx.particles[i].position;
            float distSq = r.x*r.x + r.y*r.y + r.z*r.z;
            float softDistSq = distSq + (epsilon * epsilon);
            float invDist = 1.0f / std::sqrt(softDistSq);
            float invDist3 = invDist * invDist * invDist;

            float weight = ctx.particles[j].mass * invDist3;

            // Apply Gravity
            ctx.particles[i].acceleration = ctx.particles[i].acceleration + (r * weight);

            // Apply Information Transfer (The "Payload")
            if (ctx.valueDim > 0) {
                const float* vJ = ctx.valueIn + (j * ctx.stride) + 7;
                float attnScore = weight * G_coupling;

                for(int k=0; k<ctx.valueDim; ++k) {
                    outV[k] += vJ[k] * attnScore;
                }
            }
        }
    }
}
inline void computeNbodyForces(std::vector<job::science::data::Particle> &particles)
{

    float epsilon = 0.1f;
    const size_t n = particles.size();

    for (size_t i = 0; i < n; ++i) {
        particles[i].acceleration = science::data::kEmptyVec3F;

        for (size_t j = 0; j < n; ++j) {
            if (i == j)
                continue;

            Vec3f r = particles[j].position - particles[i].position;
            float distSq = r.x * r.x + r.y * r.y + r.z * r.z;

            // FIX: Softening parameter prevents infinity when particles overlap.
            // Classic Plummer potential approach.
            float softDistSq = distSq + (epsilon * epsilon);

            float invDist  = 1.0f / std::sqrt(softDistSq);
            float invDist3 = invDist * invDist * invDist;

            // Acceleration = G * Mass / r^2 * (r_vec / r) = Mass * r_vec / r^3
            // (Note: G is applied by the Adapter later)
            Vec3f f = r * (particles[j].mass * invDist3);
            particles[i].acceleration = particles[i].acceleration + f;
        }
    }
}

} // namespace job::ai::adapters
