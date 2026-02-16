#pragma once

#include <vector>
#include <functional>

#include <job_logger.h>
#include <real_type.h>

#include <job_thread_pool.h>
#include <utils/job_parallel_for.h>

#include "job_verlet_concepts.h"

namespace job::science {


enum class VVScheme : uint8_t {
    KDK = 0,        // Kick–Drift–Kick (default)
    DKD = 1         // Drift–Kick–Drift
};

template <typename T_Particle, typename T_Vec>
class VerletIntegrator {
public:
    using AccessorFunc = std::function<T_Vec&(T_Particle&)>;
    using AccelCalculator = std::function<void(std::vector<T_Particle>&)>;

    VerletIntegrator(threads::ThreadPool::Ptr pool,
                     std::vector<T_Particle> *particles,
                     AccessorFunc getPosition,
                     AccessorFunc getVelocity,
                     AccessorFunc getAcceleration,
                     AccelCalculator accelCalculator) :
        m_pool(std::move(pool)),
        m_particles(particles),
        m_position(std::move(getPosition)),
        m_velocity(std::move(getVelocity)),
        m_acceleration(std::move(getAcceleration)),
        m_accelCalculator(std::move(accelCalculator))
    {
        // to do add aesserts ?
        if (!m_pool)
            JOB_LOG_ERROR("[VerletIntegrator] ThreadPool is null!");

        if (!m_particles)
            JOB_LOG_ERROR("[VerletIntegrator] Particle vector is null!");

        if (!m_position || !m_velocity || !m_acceleration)
            JOB_LOG_ERROR("[VerletIntegrator] One of the Accessor[Velocity, Acceleration, ] callbacks is null!");

        if (!m_accelCalculator)
            JOB_LOG_ERROR("[VerletIntegrator] AccelCalculator is null!");
    }

    // API
    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void step(T_TYPE dt, VVScheme scheme = VVScheme::KDK, bool threaded = true)
    {
        run(dt, scheme, threaded);
    }

    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void stepKDK(T_TYPE dt, bool threaded = true)
    {
        step(dt, VVScheme::KDK, threaded);
    }

    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void stepDKD(T_TYPE dt, bool threaded = true)
    {
        step(dt, VVScheme::DKD, threaded);
    }

private:

    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void run(T_TYPE dt, VVScheme scheme = VVScheme::KDK, bool threaded = true)
    {
        if (!m_pool || !m_particles)
            return;

        if (!(dt > T_TYPE(0))) {
            JOB_LOG_WARN("[VerletIntegrator] Non-positive dt: {}", dt);
            return;
        }

        if (m_particles->empty())
            return;

        switch (scheme) {
        case VVScheme::KDK:
            kick(dt, threaded);
            break;
        case VVScheme::DKD:
            drift(dt, threaded);
            break;
        }
    }

    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void kick(T_TYPE dt, bool threaded)
    {
        const T_TYPE half_dt = dt * T_TYPE(0.5);

        auto run_loop = [&](auto func) {
            if (threaded)
                job::threads::parallel_for(*m_pool, std::size_t(0), m_particles->size(), func);
            else
                for (size_t i = 0; i < m_particles->size(); ++i)
                    func(i);
        };
        run_loop([&](size_t i) {
            auto &particle      = (*m_particles)[i];

            auto &acceleration  = m_acceleration(particle);
            auto &velocity      = m_velocity(particle);
            auto &position      = m_position(particle);

            velocity = velocity + (acceleration * half_dt);
            position = position + (velocity * dt);
        });
        m_accelCalculator(*m_particles);
        run_loop([&](size_t i) {
            auto &particle      = (*m_particles)[i];
            auto &velocity      = m_velocity(particle);
            auto &acceleration  = m_acceleration(particle);
            velocity = velocity + (acceleration * half_dt);
        });
    }


    template <core::FloatScalar T_TYPE>
        requires (VerletVector<T_Vec, T_TYPE>)
    void drift(T_TYPE dt, bool threaded)
    {
        const T_TYPE half_dt = dt * T_TYPE(0.5);
        forEachVelocity(threaded, [&](size_t i){
            auto &particle      = (*m_particles)[i];
            auto &position      = m_position(particle);
            auto &velocity      = m_velocity(particle);
            position = position + (velocity * half_dt);
        });

        m_accelCalculator(*m_particles);


        forEachVelocity(threaded, [&](size_t i) {
            auto &particle      = (*m_particles)[i];
            auto &velocity      = m_velocity(particle);
            auto &acceleration  = m_acceleration(particle);
            auto &position      = m_position(particle);

            velocity = velocity + (acceleration * dt);
            position = position + (velocity * half_dt);
        });
    }

    template<typename T_FUNC>
    inline void forEachVelocity(bool threaded, T_FUNC &&f)
    {
        const size_t n = m_particles->size();
        if (threaded)
            parallel_for(*m_pool, std::size_t(0), n, std::forward<T_FUNC>(f));
        else
            for (size_t i = 0; i < n; ++i)
                f(i);
    }


    threads::ThreadPool::Ptr    m_pool;
    std::vector<T_Particle>     *m_particles{nullptr};
    AccessorFunc                m_position;
    AccessorFunc                m_velocity;
    AccessorFunc                m_acceleration;
    AccelCalculator             m_accelCalculator;
};

} // namespace job::threads
