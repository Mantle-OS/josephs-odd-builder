#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>
#include <cmath>
#include <numbers>

#include <job_logger.h>

#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>
#include <utils/job_verlet_adapters.h>
#include <utils/job_rk4_integrator.h>

#include "test_thread_science.h"

using namespace job::threads;
using namespace std::chrono_literals;


TEST_CASE("RK4 Integrator: Simple Harmonic Oscillator (Accuracy Check)", "[threading][science][rk4]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);
    std::vector<Particle> particles(1);

    // Start at x=1, v=0
    particles[0].id = 1;
    particles[0].mass = 1.0f;
    particles[0].position = {1.0f, 0.0f, 0.0f};
    particles[0].velocity = {0.0f, 0.0f, 0.0f};
    particles[0].acceleration = {0.0f, 0.0f, 0.0f};

    // Force -> Accel
    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f& {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobRK4Integrator<Particle, Vec3f, float> integrator(pool, &particles,
                                                        [](Particle &p) -> Vec3f&{
                                                            return p.position;
                                                        },
                                                        [](Particle &p) -> Vec3f&{
                                                            return p.velocity;
                                                        },
                                                        [](Particle &p) -> Vec3f&{
                                                            return p.acceleration;
                                                        },
                                                        accel_calc
                                                        );

    // Analytical solution: x(t) = A * cos(sqrt(k/m) * t) | k=1, m=1 -> omega = 1. Period = 2*PI.
    const float dt = 0.01f;
    const float target_time = 2.0f * std::numbers::pi_v<float>;
    const int steps = static_cast<int>(std::ceil(target_time / dt));

    for (int i = 0; i < steps; ++i)
        integrator.step(dt);

    auto &p = particles[0];

    // Use the *actual* final time, not the ideal period
    const float t_final = steps * dt;
    const float expected_pos = std::cos(t_final);
    const float expected_vel = -std::sin(t_final);

    REQUIRE_THAT(p.position.x, Catch::Matchers::WithinAbs(expected_pos, 1e-4f));
    REQUIRE_THAT(p.velocity.x, Catch::Matchers::WithinAbs(expected_vel, 1e-4f));
}

TEST_CASE("RK4 Integrator: Damped Oscillator (Velocity Dependent estimates)", "[threading][science][rk4]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);

    std::vector<Particle> particles(1);
    particles[0].id = 1;
    particles[0].mass = 1.0f;
    particles[0].position = {1.0f, 0.0f, 0.0f};
    particles[0].velocity = {0.0f, 0.0f, 0.0f};

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(dampedSpringForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f&{
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobRK4Integrator<Particle, Vec3f, float> integrator(pool, &particles,
                                                        [](Particle &p) -> Vec3f& {
                                                            return p.position;
                                                        },
                                                        [](Particle &p) -> Vec3f& {
                                                            return p.velocity;
                                                        },
                                                        [](Particle &p) -> Vec3f& {
                                                            return p.acceleration;
                                                        },
                                                        accel_calc
                                                        );

    const float dt = 0.1f;
    // Run for 5 seconds or should
    const int steps = 50;

    for (int i = 0; i < steps; ++i)
        integrator.step(dt);

    pool->shutdown();

    auto &p = particles[0];
    JOB_LOG_INFO("[RK4] Damped Final: Pos {}", p.position.x);

    // Physics check Amplitude must decay
    // Start was 1.0. End should be significantly less, but oscillating
    REQUIRE(std::abs(p.position.x) < 0.9f);

    // Energy check: Kinetic + Potential should strictly decrease
    float energy = 0.5f * p.velocity.lengthSq() + 0.5f * p.position.lengthSq();

    // Started with 0.5J (0.5 * k * x/\2)
    REQUIRE(energy < 0.5f);
}


TEST_CASE("RK4 Integrator: error shrinks with timestep (convergence)", "[threading][science][rk4][convergence]")
{

    // Murder mystery where the victim is "numerical accuracy" and I was hunting for the culprit
    // Victim: “perfect 4th-order convergence ratios”.
    // Culprit: “finite precision, long time, and a slightly overconfident assertion”.
    // Cause of Death: Mistaken identity—victim is alive and well!
    // Culprit: Detective's paranoia (me) and initial timestep choices
    auto make_sim = [&](float dt) {
        auto sched = std::make_shared<FifoScheduler>();
        auto pool  = ThreadPool::create(sched, 4);

        std::vector<Particle> ps(1);
        ps[0].mass     = 1.0f;
        ps[0].position = {1.0f, 0.0f, 0.0f};
        ps[0].velocity = {0.0f, 0.0f, 0.0f};

        auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                       [](const Particle &p) {
                                                                           return p.mass;
                                                                       },
                                                                       [](Particle &p) -> Vec3f& {
                                                                           return p.acceleration;
                                                                       }
                                                                       );

        JobRK4Integrator<Particle, Vec3f, float> integ(pool, &ps,
                                                       [](Particle &p) -> Vec3f& {
                                                           return p.position;
                                                       },
                                                       [](Particle &p) -> Vec3f& {
                                                           return p.velocity;
                                                       },
                                                       [](Particle &p) -> Vec3f& {
                                                           return p.acceleration;
                                                       },
                                                       accel_calc
                                                       );

        // !!!IMPORTANT!! Use a fixed time that is a multiple of all dt's (0.2, 0.1, 0.05) so we stop exactly at the same simulation time.?
        const float t_final = 6.0f;
        const int steps = static_cast<int>(std::round(t_final / dt));

        for (int i = 0; i < steps; ++i)
            integ.step(dt);

        pool->shutdown();

        // Calculate analytical position at the ACTUAL simulation time
        const float actual_time = steps * dt;
        const float expected_x = std::cos(actual_time);
        const float expected_v = -std::sin(actual_time);

        float ex = ps[0].position.x - expected_x;
        float ev = ps[0].velocity.x - expected_v;
        return std::sqrt(ex*ex + ev*ev);
    };

    // !!!IMPORTANT!! Use LARGER timesteps to ensure truncation error dominates float epsilon
    float err_dt1 = make_sim(0.2f);
    float err_dt2 = make_sim(0.1f);
    float err_dt3 = make_sim(0.05f);

    auto observed_order = [](float e_coarse, float e_fine) {
        return std::log(e_coarse / e_fine) / std::log(2.0f);
    };

    float p1 = observed_order(err_dt1, err_dt2);
    float p2 = observed_order(err_dt2, err_dt3);

    JOB_LOG_INFO("[RK4] Error dt=0.2:  {}", err_dt1);
    JOB_LOG_INFO("[RK4] Error dt=0.1:  {}", err_dt2);
    JOB_LOG_INFO("[RK4] Error dt=0.05: {}", err_dt3);
    JOB_LOG_INFO("[RK4] Order dt=0.2->0.1:  {}", p1);
    JOB_LOG_INFO("[RK4] Order dt=0.1->0.05: {}", p2);

    // With the fixed time bounds, we expect 4th order (approx 4.0). Floating point noise might lower it slightly, so > 3.5 is a strong pass.
    REQUIRE(p1 > 3.5f);
    REQUIRE(p2 > 3.5f);

    // Sanity checks: Error must strictly decrease
    REQUIRE(err_dt2 < err_dt1);
    REQUIRE(err_dt3 < err_dt2);
}

// EDGES
TEST_CASE("RK4 Integrator: Edge Cases", "[threading][science][rk4][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    {
        std::vector<Particle> empty_particles;
        auto pop_goes_the_wessel = [](std::vector<Particle>&) {
            throw std::runtime_error("Should not run");
        };

        JobRK4Integrator<Particle, Vec3f, float> integ(pool, &empty_particles,
                                                       nullptr,
                                                       nullptr,
                                                       nullptr,
                                                       pop_goes_the_wessel
                                                       );

        // "Should" return immediately
        integ.step(0.1f);
        SUCCEED("Empty particle vector handled safely");
    }

    {
        JobRK4Integrator<Particle, Vec3f, float> integ(pool,
                                                       nullptr,
                                                       nullptr,
                                                       nullptr,
                                                       nullptr,
                                                       [](auto&){
                                                       }
                                                       );
        integ.step(0.1f);
        SUCCEED("Null particle pointer handled safely");
    }

    pool->shutdown();
}


TEST_CASE("RK4 Integrator: handles many particles", "[threading][science][rk4][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 8);

    constexpr std::size_t kCount = 10000;
    std::vector<Particle> ps(kCount);

    for (std::size_t i = 0; i < kCount; ++i) {
        ps[i].id        = i;
        ps[i].mass      = 1.0f;
        // i tried  . . ..
        ps[i].position  = { float(i % 100) * 0.01f,
                          float((i / 100) % 100) * 0.01f,
                          0.0f };
        ps[i].velocity  = {0.0f, 0.0f, 0.0f};
    }

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f& {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobRK4Integrator<Particle, Vec3f, float> integ(pool, &ps,
                                                   [](Particle &p) -> Vec3f& {
                                                       return p.position;
                                                   },
                                                   [](Particle &p) -> Vec3f& {
                                                       return p.velocity;
                                                   },
                                                   [](Particle &p) -> Vec3f& {
                                                       return p.acceleration;
                                                   },
                                                   accel_calc
                                                   );

    const float dt    = 0.01f;
    const int   steps = 20;

    for (int i = 0; i < steps; ++i)
        integ.step(dt);

    // Sanity: nothing exploded numerically
    for (const auto &p : ps) {
        REQUIRE(std::isfinite(p.position.x));
        REQUIRE(std::isfinite(p.position.y));
        REQUIRE(std::isfinite(p.position.z));
    }

    pool->shutdown();
}

// CHECKPOINT: v1.0
