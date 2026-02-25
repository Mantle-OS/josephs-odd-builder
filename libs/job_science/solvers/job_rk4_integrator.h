#pragma once

#include <functional>
#include <vector>

#include <job_logger.h>
#include <real_type.h>

#include <job_thread_pool.h>
#include <utils/job_parallel_for.h>

#include "job_barns_hut_concept.h"

// Confused me too you're not the one with the drunk cousin
// Here is a doc's/video that I was using to help understand this.
// * https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods
// * https://aquaulb.github.io/book_solving_pde_mooc/solving_pde_mooc/notebooks/02_TimeIntegration/02_02_RungeKutta.html stupid python . . . .
// * https://www.youtube.com/watch?v=kUcc8vAgoQ0

namespace job::science {

template <typename T_Particle, typename T_Vec, core::FloatScalar T_Scalar>
    requires BarnesHutVector<T_Vec, T_Scalar>
class JobRK4Integrator {
public:
    using AccessorFunc    = std::function<T_Vec&(T_Particle&)>;
    using AccelCalculator = std::function<void(std::vector<T_Particle>&)>;

    JobRK4Integrator(threads::ThreadPool::Ptr pool, std::vector<T_Particle> *particles, AccessorFunc getPos, AccessorFunc getVel, AccessorFunc getAcc,
                     AccelCalculator accelCalc, bool parallel = true) :
        m_pool(std::move(pool)),
        m_particles(particles),
        m_getPos(std::move(getPos)),
        m_getVel(std::move(getVel)),
        m_getAcc(std::move(getAcc)),
        m_accelCalc(std::move(accelCalc)),
        m_parallel(parallel)
    {

        if (!m_pool)
            JOB_LOG_WARN("[JobRK4Integrator] ThreadPool is null!");

        if (!m_particles)
            JOB_LOG_WARN("[JobRK4Integrator] particles pointer is null (step() will no-op).");
        //NOTE: in c++26 we can check reflections to make sure that there are infact the needed memebers. CAN NOT WAIT !!!!
    }

    void step(T_Scalar dt)
    {
        if (!m_particles || m_particles->empty())
            return;

        if (!(dt > T_Scalar(0))) {
            JOB_LOG_WARN("[JobRK4Integrator] Non-positive dt: {}", dt);
            return;
        }

        const std::size_t N = m_particles->size();
        resizeScratch(N);

        auto execute = [&](auto &&kernel) {
            if (m_parallel) {
                parallel_for(*m_pool, std::size_t{0}, N, kernel);
            } else {
                // Serial Fallback (Safe inside another parallel_for)
                for (std::size_t i = 0; i < N; ++i)
                    kernel(i);
            }
        };

        execute([&](std::size_t i) {
            m_scratch[i] = (*m_particles)[i];
            m_x0[i] = m_getPos(m_scratch[i]);
            m_v0[i] = m_getVel(m_scratch[i]);
            m_accumX[i] = T_Vec{};
            m_accumV[i] = T_Vec{};
        });

        const T_Scalar halfDt = dt * T_Scalar(0.5);
        const T_Scalar oneSixthDt = dt / T_Scalar(6.0);

        // k1 | x0, v0 cal a1 based on x0, v0 inside m_scratch
        m_accelCalc(m_scratch);
        execute([&](std::size_t i) {
            T_Vec k1_dx = m_v0[i];
            T_Vec k1_dv = m_getAcc(m_scratch[i]);

            m_accumX[i] = m_accumX[i] + k1_dx;
            m_accumV[i] = m_accumV[i] + k1_dv;

            m_getPos(m_scratch[i]) = m_x0[i] + (k1_dx * halfDt);
            m_getVel(m_scratch[i]) = m_v0[i] + (k1_dv * halfDt);
        });

        // k2 | x0 + k1*0.5dt)
        m_accelCalc(m_scratch);
        execute([&](std::size_t i) {
            T_Vec k2_dx = m_getVel(m_scratch[i]);
            T_Vec k2_dv = m_getAcc(m_scratch[i]);

            m_accumX[i] = m_accumX[i] + (k2_dx * T_Scalar(2.0));
            m_accumV[i] = m_accumV[i] + (k2_dv * T_Scalar(2.0));

            m_getPos(m_scratch[i]) = m_x0[i] + (k2_dx * halfDt);
            m_getVel(m_scratch[i]) = m_v0[i] + (k2_dv * halfDt);
        });

        // k3 | x0 + k2*0.5dt
        m_accelCalc(m_scratch);
        execute([&](std::size_t i) {
            T_Vec k3_dx = m_getVel(m_scratch[i]);
            T_Vec k3_dv = m_getAcc(m_scratch[i]);

            m_accumX[i] = m_accumX[i] + (k3_dx * T_Scalar(2.0));
            m_accumV[i] = m_accumV[i] + (k3_dv * T_Scalar(2.0));

            m_getPos(m_scratch[i]) = m_x0[i] + (k3_dx * dt);
            m_getVel(m_scratch[i]) = m_v0[i] + (k3_dv * dt);
        });

        // k4 | x0 + k3*dt
        m_accelCalc(m_scratch);

        execute([&](std::size_t i) {
            T_Vec k4_dx = m_getVel(m_scratch[i]);
            T_Vec k4_dv = m_getAcc(m_scratch[i]);

            m_accumX[i] = m_accumX[i] + k4_dx;
            m_accumV[i] = m_accumV[i] + k4_dv;

            auto &p = (*m_particles)[i];
            auto &x = m_getPos(p);
            auto &v = m_getVel(p);

            x = m_x0[i] + (m_accumX[i] * oneSixthDt);
            v = m_v0[i] + (m_accumV[i] * oneSixthDt);
        });

        // keep real particle accelerations in sync with final state. Test to see if thiws is too heavy
        if (m_accelCalc)
            m_accelCalc(*m_particles);

    }


    void step_n(std::size_t iterations, T_Scalar dt)
    {
        for (std::size_t i = 0; i < iterations; ++i)
            step(dt);
    }

private:
    void resizeScratch(std::size_t N)
    {
        if (m_scratch.size() != N) {
            m_scratch.resize(N);
            m_x0.resize(N);
            m_v0.resize(N);
            m_accumX.resize(N);
            m_accumV.resize(N);
        }
    }

    threads::ThreadPool::Ptr    m_pool;
    std::vector<T_Particle>     *m_particles;
    AccessorFunc                m_getPos;
    AccessorFunc                m_getVel;
    AccessorFunc                m_getAcc;
    AccelCalculator             m_accelCalc;
    // Snapshot of state at t=0
    std::vector<T_Vec>          m_x0;
    std::vector<T_Vec>          m_v0;
    // Accumulators for RK4 weighted sums
    std::vector<T_Vec>          m_accumX;
    std::vector<T_Vec>          m_accumV;
    // Scratch particles used for intermediate force calculations
    std::vector<T_Particle>     m_scratch;
    bool                        m_parallel{true};
};

} // namespace job::threads

