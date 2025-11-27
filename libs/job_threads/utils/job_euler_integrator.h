#pragma once

#include <vector>
#include <functional>

#include <job_logger.h>
#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"
#include "utils/job_verlet_concepts.h"

namespace job::threads {

// Euler is the slightly drunk cousin of RK4: less accurate, but incredibly handy when you just want to see “are my forces even pointing the right way?”.
// Semi-Implicit Euler (also called Symplectic Euler or Euler-Cromer):
// 1. v_{n+1} = v_n + a(x_n) * dt
// 2. x_{n+1} = x_n + v_{n+1} * dt
//
// This is better than explicit Euler for Hamiltonian systems (springs, orbits)
// because it approximately conserves energy. Explicit Euler *adds* energy over time,
// causing orbits to spiral outward.
//
// For simple harmonic motion (spring), semi-implicit Euler has bounded error,
// while explicit Euler diverges.
// Use this when:
// - Prototyping physics (forces pointing the right way?)
// - Stable-ish simulation is more important than accuracy
// - You're too lazy to implement RK4

template <typename T_Particle, typename T_Vec, FloatScalar T_Scalar>
    requires VecOps<T_Vec, T_Scalar>
class JobEulerIntegrator {
public:
    using AccessorFunc    = std::function<T_Vec &(T_Particle &)>;
    using AccelCalculator = std::function<void(std::vector<T_Particle> &)>;

    JobEulerIntegrator(ThreadPool::Ptr pool, std::vector<T_Particle> *particles, AccessorFunc getPos, AccessorFunc getVel, AccessorFunc getAcc, AccelCalculator accelCalc) :
        m_pool(std::move(pool)),
        m_particles(particles),
        m_getPos(std::move(getPos)),
        m_getVel(std::move(getVel)),
        m_getAcc(std::move(getAcc)),
        m_accelCalc(std::move(accelCalc))
    {
        if (!m_pool)
            JOB_LOG_WARN("[JobEulerIntegrator] ThreadPool is null That is not Good !");
    }

    void step(T_Scalar dt)
    {
        if (!m_particles || m_particles->empty())
            return;

        if (!(dt > T_Scalar(0))) {
            JOB_LOG_WARN("[JobEulerIntegrator] Non-positive dt: {}", dt);
            return;
        }

        if (!m_pool) {
            stepSerial(dt);
            return;
        }

        // F = m a → refresh acceleration for this frame
        if (m_accelCalc)
            m_accelCalc(*m_particles);
        else
            JOB_LOG_WARN("[JobEulerIntegrator] AccelCalculator is null; skipping step.");

        auto &ps = *m_particles;
        const std::size_t N = ps.size();

        parallel_for(*m_pool, std::size_t{0}, N, [&](std::size_t i) {
            T_Particle &p = ps[i];

            auto &x = m_getPos(p);
            auto &v = m_getVel(p);
            auto &a = m_getAcc(p);

            v = v + a * dt;
            x = x + v * dt;
        });
    }

    void step_n(std::size_t iterations, T_Scalar dt)
    {
        for (std::size_t i = 0; i < iterations; ++i)
            step(dt);
    }

private:
    void stepSerial(T_Scalar dt)
    {
        if (!m_particles)
            return;

        m_accelCalc(*m_particles);

        for (auto &p : *m_particles) {
            auto &x = m_getPos(p);
            auto &v = m_getVel(p);
            auto &a = m_getAcc(p);

            v = v + a * dt;
            x = x + v * dt;
        }
    }

    ThreadPool::Ptr         m_pool;
    std::vector<T_Particle> *m_particles{nullptr};
    AccessorFunc            m_getPos;
    AccessorFunc            m_getVel;
    AccessorFunc            m_getAcc;
    AccelCalculator         m_accelCalc;
};

} // namespace job::threads
// CHECKPOINT: v1.1
