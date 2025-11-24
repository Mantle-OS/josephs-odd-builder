#pragma once

#include <vector>
#include <functional>
#include "job_logger.h"
#include "job_thread_pool.h"
#include "utils/job_parallel_for.h"
#include "utils/job_verlet_concepts.h"

namespace job::threads {

// KDK = Kick–Drift–Kick (default)
// DKD = Drift–Kick–Drift
enum class VVScheme {
    KDK = 0,
    DKD
};

template <typename T_Particle, typename T_Vec>
class VerletIntegrator {
public:
    using AccessorFunc    = std::function<T_Vec&(T_Particle&)>;
    using AccelCalculator = std::function<void(std::vector<T_Particle>&)>;

    VerletIntegrator(ThreadPool::Ptr pool, std::vector<T_Particle>* particles,
                     AccessorFunc get_pos, AccessorFunc get_vel, AccessorFunc get_acc,
                     AccelCalculator accel_calc) :
        m_pool(std::move(pool)),
        m_particles(particles),
        m_get_pos(std::move(get_pos)),
        m_get_vel(std::move(get_vel)),
        m_get_acc(std::move(get_acc)),
        m_accel_calculator(std::move(accel_calc))
    {
        if (!m_pool)
            JOB_LOG_ERROR("[VerletIntegrator] ThreadPool is null!");

        if (!m_particles)
            JOB_LOG_ERROR("[VerletIntegrator] Particle vector is null!");

        if (!m_get_pos || !m_get_vel || !m_get_acc)
            JOB_LOG_ERROR("[VerletIntegrator] Accessor func is null!");

        if (!m_accel_calculator)
            JOB_LOG_ERROR("[VerletIntegrator] AccelCalculator is null!");
    }

    template <FloatScalar T_TYPE>
    requires (VecOps<T_Vec, T_TYPE>)
    void step(T_TYPE dt, VVScheme scheme = VVScheme::KDK)
    {
        if (!m_pool || !m_particles)
            return;

        if (!(dt > T_TYPE(0))) {
            JOB_LOG_WARN("[VerletIntegrator] Non-positive dt: {}", dt);
            return;
        }

        if (m_particles->empty())
            return;

        const T_TYPE half_dt = dt * T_TYPE(0.5);
        switch (scheme) {
        case VVScheme::KDK:
            // v(t+1/2 dt) = v(t) + a(t) * 1/2 dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &v = m_get_vel(p);
                auto &a = m_get_acc(p);
                v = v + (a * half_dt);
            });

            // x(t+dt) = x(t) + v(t+1/2 dt) * dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &x = m_get_pos(p);
                auto &v = m_get_vel(p);
                x = x + (v * dt);
            });

            // a(t+dt)
            m_accel_calculator(*m_particles);

            // v(t+dt) = v(t+1/2 dt) + a(t+dt) * 1/2 dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &v = m_get_vel(p);
                auto &a = m_get_acc(p);
                v = v + (a * half_dt);
            });
            break;

        case VVScheme::DKD:
            // x(t+1/2 dt) = x(t) + v(t) * 1/2 dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &x = m_get_pos(p);
                auto &v = m_get_vel(p);
                x = x + (v * half_dt);
            });

            // a(t+1/2 dt) from x(t+1/2 dt)
            m_accel_calculator(*m_particles);

            // v(t+dt) = v(t) + a(t+1/2 dt) * dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &v = m_get_vel(p);
                auto &a = m_get_acc(p);
                v = v + (a * dt);
            });

            // x(t+dt) = x(t+1/2 dt) + v(t+dt) * 1/2 dt
            parallel_for(*m_pool, std::size_t(0), m_particles->size(), [&](std::size_t i){
                auto &p = (*m_particles)[i];
                auto &x = m_get_pos(p);
                auto &v = m_get_vel(p);
                x = x + (v * half_dt);
            });
            break;
        }
    }

    // wrappers
    template <FloatScalar T_TYPE> requires (VecOps<T_Vec, T_TYPE>)
    void kickDriftKick(T_TYPE dt)
    {
        step(dt, VVScheme::KDK);
    }

    template <FloatScalar T_TYPE> requires (VecOps<T_Vec, T_TYPE>)
    void driftKickDrift(T_TYPE dt)
    {
        step(dt, VVScheme::DKD);
    }

private:
    ThreadPool::Ptr         m_pool;
    std::vector<T_Particle> *m_particles{nullptr};
    AccessorFunc            m_get_pos;
    AccessorFunc            m_get_vel;
    AccessorFunc            m_get_acc;
    AccelCalculator         m_accel_calculator;
};

} // namespace job::threads
