#pragma once

#include <cstring>
#include <vector>
#include <cmath>
#include <algorithm>


#include <real_type.h>
// Architecture
#include <job_thread_pool.h>
#include <job_parallel_for.h>

#include "aligned_allocator.h"

// Science (The Solver)
#include <vec3f.h>
#include <particle.h>
#include <job_fmm_integrator.h>

#include "aligned_allocator.h"
namespace job::ai::machine::att {


struct TokenTraits {
    using Vec3 = science::data::Vec3f;
    using Real = core::real_t;
    struct Body {
        Vec3        position;
        Vec3        acceleration; // The "Context Vector" result
        Real        mass;         // The "Attention Weight"
        uint32_t    original_index; // To map back to tensor
    };

    // Multipole Expansion Order (P=0 is fast, P=3 is accurate)
    static constexpr int P = 3;
};

class FmmAdapter {
public:
    using Alloc = job::ai::base::AlignedAllocator<core::real_t, 64>;
    using FmmSolver = job::threads::JobFmmEngine<
        TokenTraits::Body,
        TokenTraits::Vec3,
        TokenTraits::Real,
        TokenTraits
        >;

    struct Config {
        core::real_t    theta         = core::real_t(0.5);      // Accuracy vs Speed knob (0.5 is standard)
        uint32_t        max_leaf      = 64;                     // Particles per leaf
        core::real_t    gravity_const = core::real_t(1.0);      // G
    };

    explicit FmmAdapter(Config cfg) :
        m_config(cfg)
    {
    }

    // -------------------------------------------------------------------------
    // COMPUTE ATTENTION (PHYSICS MODE)
    // -------------------------------------------------------------------------
    // Instead of Softmax(QK^T) * V, we compute:
    // Force = G * Sum( Mass_k * (Pos_k - Pos_q) / |r|^3 )
    //
    // Q: [Batch, Seq, HeadDim] -> Interpreted as Positions
    // K: [Batch, Seq, HeadDim] -> Interpreted as Mass sources
    // Output: [Batch, Seq, HeadDim] -> Writes Force into first 3 dims
    // -------------------------------------------------------------------------
    void compute_forces(
        job::threads::ThreadPool &pool,
        int batch_size,
        int seq_len,
        int head_dim,
        const core::real_t* __restrict__ Q,
        const core::real_t* __restrict__ K,
        core::real_t* __restrict__ Output)
    {
        // 1. Prepare Particles (Scatter)
        // We currently run one FMM tree per batch item.
        // (Optimization: We could put all batches in one massive tree if far apart,
        // but parallelizing over batches is cleaner).

        job::threads::parallel_for(pool, size_t{0}, size_t(batch_size), [&](size_t b) {

            // Local scratchpad for this batch (avoid allocations in hot loop if possible,
            // but FMM constructs a tree anyway, so vector resize is minor relative cost).
            std::vector<TokenTraits::Body> particles(seq_len);

            const core::real_t* q_ptr = Q + (b * seq_len * head_dim);
            const core::real_t* k_ptr = K + (b * seq_len * head_dim);
            core::real_t* out_ptr     = Output + (b * seq_len * head_dim);

            // 1a. Encoder: Tensor -> Particle
            // We use the first 3 dimensions as X, Y, Z.
            // We use the 4th dimension (or norm) as Mass.
            for (int i = 0; i < seq_len; ++i) {
                int idx = i * head_dim;

                // Position from Q (Where we are looking from)
                particles[i].position.x = q_ptr[idx + 0];
                particles[i].position.y = q_ptr[idx + 1];
                particles[i].position.z = head_dim > 2 ? q_ptr[idx + 2] : 0.0f;

                // Mass from K (How important this memory is)
                // If head_dim > 3, use 4th dim, else use 1.0f or norm
                particles[i].mass = (head_dim > 3) ? std::abs(k_ptr[idx + 3]) : 1.0f;

                particles[i].acceleration = {0,0,0};
                particles[i].original_index = i;
            }

            // 2. The Heavy Lifting: Fast Multipole Method
            // This is O(N) instead of O(N^2)
            typename FmmSolver::Params params;
            params.theta = m_config.theta;
            params.maxLeafSize = m_config.max_leaf;

            // Note: We create a temporary solver per batch.
            // If FmmEngine allocates heavily, we should make this persistent thread-local.
            FmmSolver engine(pool, params);

            // "Self-Gravity" - The tokens attract each other based on semantic distance
            engine.compute(particles);

            // Decoder: Particle Force -> Output Tensor
            // We write the computed 3D force back into the first 3 dimensions of Output.
            // This "Context Vector" represents the pull of relevant information.
            for (int i = 0; i < seq_len; ++i) {
                int idx = i * head_dim;

                // Apply Gravity Constant
                core::real_t fx = particles[i].acceleration.x * m_config.gravity_const;
                core::real_t fy = particles[i].acceleration.y * m_config.gravity_const;
                core::real_t fz = particles[i].acceleration.z * m_config.gravity_const;

                // Residual Add or Overwrite? Usually overwrite for an Attention Block output.
                out_ptr[idx + 0] = fx;
                out_ptr[idx + 1] = fy;
                if (head_dim > 2) out_ptr[idx + 2] = fz;

                // Zero out remaining dimensions?
                // Or maybe spread the energy? For now, we leave them (or zero them).
                // Let's zero them to be clean.
                if (head_dim > 3) {
                    std::memset(out_ptr + idx + 3, 0, (head_dim - 3) * sizeof(core::real_t));
                }
            }
        });
    }

private:
    Config m_config;
};

} // namespace
