#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>
#include <cmath>
#include <numbers>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>

#include <utils/job_verlet_adapters.h>
#include <utils/job_euler_integrator.h>

#include "test_thread_science.h"

using namespace job::threads;
using namespace std::chrono_literals;

// BUELLER ... BUELLER .... ELUER  ...

TEST_CASE("Euler Integrator: SHO(Simple harmonic oscillator) stays bounded and roughly periodic", "[threading][science][euler][sho]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    std::vector<Particle> particles(1);
    particles[0].id           = 1;
    particles[0].mass         = 1.0f;
    particles[0].position     = {1.0f, 0.0f, 0.0f};
    particles[0].velocity     = {0.0f, 0.0f, 0.0f};
    particles[0].acceleration = {0.0f, 0.0f, 0.0f};

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f & {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobEulerIntegrator<Particle, Vec3f, float> integrator( pool, &particles,
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.position;
                                                          },
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.velocity;
                                                          },
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.acceleration;
                                                          },
                                                          accel_calc
                                                          );

    // !!!! IMPORTANT !!!! Small dt so Euler doesn’t immediately launch our particle into the sun.
    const float dt         = 0.001f;
    const float targetTime = 2.0f * std::numbers::pi_v<float>; // one period (ω=1)
    const int   steps      = static_cast<int>(std::ceil(targetTime / dt));

    const float E0 = 0.5f * particles[0].position.lengthSq()
                     + 0.5f * particles[0].velocity.lengthSq();

    for (int i = 0; i < steps; ++i)
        integrator.step(dt);

    pool->shutdown();

    auto &p = particles[0];

    JOB_LOG_INFO("[Euler] SHO Final: Pos ({}, {}, {}), Vel ({}, {}, {})",
                 p.position.x, p.position.y, p.position.z,
                 p.velocity.x, p.velocity.y, p.velocity.z);

    const float E1 = 0.5f * p.position.lengthSq()
                     + 0.5f * p.velocity.lengthSq();

    REQUIRE_THAT(p.position.x, Catch::Matchers::WithinAbs(1.0f, 0.05f));
    REQUIRE_THAT(p.velocity.x, Catch::Matchers::WithinAbs(0.0f, 0.1f));
    REQUIRE(E1 < 2.0f * E0);
}

TEST_CASE("Euler Integrator: Damped oscillator: energy should go down, not up", "[threading][science][euler][damped]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    std::vector<Particle> particles(1);
    particles[0].id           = 1;
    particles[0].mass         = 1.0f;
    particles[0].position     = {1.0f, 0.0f, 0.0f};
    particles[0].velocity     = {0.0f, 0.0f, 0.0f};
    particles[0].acceleration = {0.0f, 0.0f, 0.0f};

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(dampedSpringForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f & {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobEulerIntegrator<Particle, Vec3f, float> integrator(pool, &particles,
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.position;
                                                          },
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.velocity;
                                                          },
                                                          [](Particle &p) -> Vec3f & {
                                                              return p.acceleration;
                                                          },
                                                          accel_calc
                                                          );

    const float dt    = 0.01f;
    const float tEnd  = 5.0f;
    const int   steps = static_cast<int>(tEnd / dt);

    const float initialEnergy = 0.5f * particles[0].position.lengthSq()
                                + 0.5f * particles[0].velocity.lengthSq(); // 0.5 J

    for (int i = 0; i < steps; ++i)
        integrator.step(dt);

    pool->shutdown();

    auto &p = particles[0];

    const float finalEnergy = 0.5f * p.position.lengthSq() + 0.5f * p.velocity.lengthSq();

    JOB_LOG_INFO("[Euler] Damped Final: Pos {}, Energy {}", p.position.x, finalEnergy);

    REQUIRE(std::abs(p.position.x) < 0.9f);
    REQUIRE(finalEnergy < initialEnergy);
}


// Edge cases
TEST_CASE("Euler Integrator: edge cases behave safely", "[threading][science][euler][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 1);

    SECTION("Empty particle vector does nothing and does not call accelCalc")
    {
        std::vector<Particle> empty;
        bool                  called = false;

        auto accel_calc = [&](std::vector<Particle> &) {
            called = true;
        };

        JobEulerIntegrator<Particle, Vec3f, float> integrator(pool, &empty,
                                                              [](Particle &p) -> Vec3f & {
                                                                  return p.position;
                                                              },
                                                              [](Particle &p) -> Vec3f & {
                                                                  return p.velocity;
                                                              },
                                                              [](Particle &p) -> Vec3f & {
                                                                  return p.acceleration;
                                                              },
                                                              accel_calc
                                                              );

        integrator.step(0.1f);
        REQUIRE(called == false);
    }

    SECTION("Null particle pointer is ignored safely")
    {
        auto accel_calc = [](std::vector<Particle> &) {
            // If this ever runs, something went very wrong.
            throw std::runtime_error("Should not be called for null particles");
        };

        JobEulerIntegrator<Particle, Vec3f, float> integrator(
            pool,
            nullptr,
            [](Particle &p) -> Vec3f & {
                return p.position;
            },
            [](Particle &p) -> Vec3f & {
                return p.velocity;
            },
            [](Particle &p) -> Vec3f & {
                return p.acceleration;
            },
            accel_calc
            );

        // Should early-out without touching anything.
        integrator.step(0.1f);
        SUCCEED("Null particle pointer handled without crash");
    }

    SECTION("Null pool falls back to serial path")
    {
        std::vector<Particle> particles(1);
        particles[0].mass         = 1.0f;
        particles[0].position     = {0.0f, 0.0f, 0.0f};
        particles[0].velocity     = {0.0f, 0.0f, 0.0f};
        particles[0].acceleration = {0.0f, 0.0f, 0.0f};

        auto accel_calc = [](std::vector<Particle> &ps) {
            // Constant "gravity" along -Y
            for (auto &p : ps)
                p.acceleration = {0.0f, -9.8f, 0.0f};
        };

        JobEulerIntegrator<Particle, Vec3f, float> integrator(
            nullptr, // <- THE IMPORTANT BIT: exercise stepSerial()
            &particles,
            [](Particle &p) -> Vec3f & {
                return p.position;
            },
            [](Particle &p) -> Vec3f & {
                return p.velocity;
            },
            [](Particle &p) -> Vec3f & {
                return p.acceleration;
            },
            accel_calc
            );

        const float dt = 1.0f;
        integrator.step(dt);

        auto &p = particles[0];

        // Semi-implicit Euler:
        // v1 = v0 + a*dt = -9.8
        // x1 = x0 + v1*dt = -9.8
        REQUIRE_THAT(p.velocity.y, Catch::Matchers::WithinAbs(-9.8f, 0.001f));
        REQUIRE_THAT(p.position.y, Catch::Matchers::WithinAbs(-9.8f, 0.001f));
    }

    pool->shutdown();
}


TEST_CASE("Euler Integrator: 1st-order convergence", "[threading][science][euler][convergence]")
{
    auto make_sim = [&](float dt) {
        auto sched = std::make_shared<FifoScheduler>();
        auto pool  = ThreadPool::create(sched, 4);

        std::vector<Particle> ps(1);
        ps[0].mass     = 1.0f;
        ps[0].position = {1.0f, 0.0f, 0.0f};
        ps[0].velocity = {0.0f, 0.0f, 0.0f};

        auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>( springForce,
                                                                       [](const Particle &p) {
                                                                           return p.mass;
                                                                       },
                                                                       [](Particle &p) -> Vec3f& {
                                                                           return p.acceleration;
                                                                       }
                                                                       );

        JobEulerIntegrator<Particle, Vec3f, float> integ(pool, &ps,
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

        const float t_final = 2.0f;  // YOU AGAIN  ...... Short time for convergence test. see test_rk4 for the murder mystery
        const int steps = static_cast<int>(std::round(t_final / dt));

        for (int i = 0; i < steps; ++i)
            integ.step(dt);

        pool->shutdown();

        const float actual_time = steps * dt;
        const float expected_x = std::cos(actual_time);
        const float expected_v = -std::sin(actual_time);

        float ex = ps[0].position.x - expected_x;
        float ev = ps[0].velocity.x - expected_v;
        return std::sqrt(ex*ex + ev*ev);
    };

    float err_dt1 = make_sim(0.1f);
    float err_dt2 = make_sim(0.05f);
    float err_dt3 = make_sim(0.025f);

    auto observed_order = [](float e_coarse, float e_fine) {
        return std::log(e_coarse / e_fine) / std::log(2.0f);
    };

    float p1 = observed_order(err_dt1, err_dt2);
    float p2 = observed_order(err_dt2, err_dt3);

    JOB_LOG_INFO("[Euler] Error dt=0.1:  {}", err_dt1);
    JOB_LOG_INFO("[Euler] Error dt=0.05: {}", err_dt2);
    JOB_LOG_INFO("[Euler] Error dt=0.025: {}", err_dt3);
    JOB_LOG_INFO("[Euler] Order dt=0.1->0.05:  {}", p1);
    JOB_LOG_INFO("[Euler] Order dt=0.05->0.025: {}", p2);

    // if your not 1st(order) your last - Ricky Bobby
    REQUIRE(p1 > 0.8f);  // Pig Slop on floats
    REQUIRE(p1 < 1.5f);  // Clean Pig pen for the 2nd-order
    REQUIRE(p2 > 0.8f);
    REQUIRE(p2 < 1.5f);

    // It's about to go down ......
    REQUIRE(err_dt2 < err_dt1);
    REQUIRE(err_dt3 < err_dt2);
}

TEST_CASE("Euler Integrator: handles many particles", "[threading][science][euler][stress]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 8);

    constexpr std::size_t kCount = 10000;
    std::vector<Particle> ps(kCount);

    for (std::size_t i = 0; i < kCount; ++i) {
        ps[i].id       = i;
        ps[i].mass     = 1.0f;
        ps[i].position = {float(i % 100) * 0.01f,
                          float((i / 100) % 100) * 0.01f,
                          0.0f};
        ps[i].velocity = {0.0f, 0.0f, 0.0f};
    }

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>( springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f& {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

    JobEulerIntegrator<Particle, Vec3f, float> integ(pool, &ps,
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

    const float dt    = 0.001f;  // Look at this little guy isn't it a cute little dt
    const int   steps = 100;

    for (int i = 0; i < steps; ++i)
        integ.step(dt);

    // Cross finglers the bomb is not goning to go off
    for (const auto &p : ps) {
        REQUIRE(std::isfinite(p.position.x));
        REQUIRE(std::isfinite(p.velocity.x));
    }

    pool->shutdown();
}


// CHECKPOINT v1.1
