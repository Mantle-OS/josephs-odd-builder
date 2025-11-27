#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>
#include <cmath>

// test data
#include "test_thread_science.h"

#include <job_logger.h>

#include <job_thread_pool.h>
#include <sched/job_work_stealing_scheduler.h>
#include <utils/job_verlet_adapters.h>
#include <utils/job_verlet_integrator.h>

using namespace job::threads;
using namespace std::chrono_literals;

using Integrator = VerletIntegrator<Particle, Vec3f>;

TEST_CASE("VerletIntegrator runs KDK and DKD with generic (POD) structs", "[threading][science][verlet]")
{
    const size_t numThreads = 4;
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(numThreads);
    auto pool = ThreadPool::create(scheduler, numThreads);
    REQUIRE(pool != nullptr);

    // Two particle's lists to compare KDK vs DKD
    std::vector<Particle> particles_kdk;
    std::vector<Particle> particles_dkd;

    // A single particle, offset from center, zero velocity
    Particle p;
    p.id = 1;
    p.position = {1.0f, 0.0f, 0.0f};
    p.velocity = {0.0f, 0.0f, 0.0f};
    // this will be set by the adaptor
    p.acceleration = {0.0f, 0.0f, 0.0f};
    p.mass = 1.0f;

    particles_kdk.push_back(p);
    particles_dkd.push_back(p);

    ///////////////////////////////////////////////////////////////////////////
    // This is the magic. We adapt our SimpleSpringForce function
    // into the AccelCalculator the integrator expects.
    ///////////////////////////////////////////////////////////////////////////
    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                               [](const Particle &p) {
                                                                   return p.mass;
                                                               },         // GetMass
                                                               [](Particle &p) -> Vec3f& {
                                                                   return p.acceleration;
                                                               } // GetAcc
                                                               );

    // Integrators
    using Integrator = VerletIntegrator<Particle, Vec3f>;

    Integrator sim_kdk(pool, &particles_kdk,
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

    Integrator sim_dkd(pool, &particles_dkd,
                       [](Particle &p) -> Vec3f& {
                           return p.position;
                       },
                       [](Particle &p) -> Vec3f& {
                           return p.velocity;
                       },
                       [](Particle &p) -> Vec3f& {
                           return p.acceleration; },
                       accel_calc
                       );


    const float dt = 0.01f;
    const int steps = 10000;

    JOB_LOG_INFO("[VELOCITY VERLET]: SCIENCE TIME Running {} loops on the simulation", steps);
    for (int i = 0; i < steps; ++i) {
        sim_kdk.step(dt, VVScheme::KDK);
        sim_dkd.step(dt, VVScheme::DKD);
    }

    pool->shutdown();

    // asserstions
    REQUIRE(particles_kdk.size() == 1);
    REQUIRE(particles_dkd.size() == 1);

    auto &p_kdk = particles_kdk[0];
    auto &p_dkd = particles_dkd[0];

    JOB_LOG_INFO("[VELOCITY VERLET]: KDK Pos: {} , Vel: {}" , p_kdk.position.x, p_kdk.velocity.x);
    JOB_LOG_INFO("[VELOCITY VERLET]: DKD Pos: {} , Vel: {}" , p_dkd.position.x, p_dkd.velocity.x);

    // cos(100.0) = 0.862318...
    const float expected_pos = std::cos(100.0f);

    REQUIRE_THAT(p_kdk.position.x, Catch::Matchers::WithinAbs(expected_pos, 0.01));
    REQUIRE_THAT(p_dkd.position.x, Catch::Matchers::WithinAbs(expected_pos, 0.01));

    // REQUIRE_THAT(p_kdk.position.x, Catch::Matchers::WithinAbs(p_dkd.position.x, 0.001));
    REQUIRE_THAT(p_kdk.position.x, Catch::Matchers::WithinAbs(p_dkd.position.x, 0.005));

    // They should NOT be at the start position
    REQUIRE_FALSE(p_kdk.position.x == 1.0f);
}


TEST_CASE("VerletIntegrator conserves energy (Symplectic) for Simple Harmonic Oscillator", "[threading][science][verlet][energy]")
{
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(1);
    auto pool = ThreadPool::create(scheduler, 1);

    std::vector<Particle> particles;
    Particle p;
    p.id = 1;
    p.position = {10.0f, 0.0f, 0.0f}; // Displaced
    p.velocity = {0.0f, 5.0f, 0.0f};  // Initial velocity
    p.acceleration = {0.0f, 0.0f, 0.0f};
    p.mass = 2.0f;
    particles.push_back(p);

    // Spring Constant from springForce in test_thread_science.h is 1.0f
    const float k_spring = 1.0f;

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                               [](const Particle &p) {
                                                                   return p.mass;
                                                               },
                                                               [](Particle &p) -> Vec3f& {
                                                                   return p.acceleration;
                                                               }
                                                               );

    Integrator sim(pool, &particles,
                   [](Particle &p) -> Vec3f& {
                       return p.position;
                   },
                   [](Particle &p) -> Vec3f& {
                       return p.velocity;
                   },
                   [](Particle &p) -> Vec3f& {
                       return p.acceleration; },
                   accel_calc
                   );

    double initial_energy = calculateTotalEnergy(particles, k_spring);
    const float dt = 0.01f;
    const int steps = 5000;

    for(int i=0; i < steps; ++i)
        sim.step(dt, VVScheme::KDK);


    double final_energy = calculateTotalEnergy(particles, k_spring);

    JOB_LOG_INFO("[Verlet Energy] Initial: {}, Final: {}", initial_energy, final_energy);

    // Symplectic integrators should keep energy bounded. It will oscillate slightly but shouldn't drift significantly like Euler.
    // We allow a small epsilon for floating point accumulation over 5000 steps.
    REQUIRE_THAT(final_energy, Catch::Matchers::WithinRel(initial_energy, 0.001)); // 0.1% tolerance

    pool->shutdown();
}

TEST_CASE("VerletIntegrator Determinism: Single Thread vs Thread Pool", "[threading][science][verlet][concurrency]")
{
    const size_t particle_count = 1000;

    std::vector<Particle> particles_serial(particle_count);
    std::vector<Particle> particles_parallel(particle_count);

    for(size_t i=0; i < particle_count; ++i) {
        Particle p;
        p.id = i;
        p.position = {static_cast<float>(i), static_cast<float>(i) * 0.1f, 0.0f};
        p.velocity = {0.0f, 0.0f, 0.0f};
        p.mass = 1.0f;
        particles_serial[i] = p;
        particles_parallel[i] = p;
    }

    {
        auto sched1 = std::make_shared<JobWorkStealingScheduler>(1);
        auto pool1 = ThreadPool::create(sched1, 1);

        auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f& {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

        VerletIntegrator<Particle, Vec3f> sim1(pool1, &particles_serial,
                                               [](Particle &p) -> Vec3f& {
                                                   return p.position;
                                               },
                                               [](Particle &p) -> Vec3f& {
                                                   return p.velocity;
                                               },
                                               [](Particle &p) -> Vec3f& {
                                                   return p.acceleration;
                                               },
                                               accel_calc);

        const float dt = 0.01f;
        for(int i=0; i<100; ++i)
            sim1.step(dt);
        pool1->shutdown();
    }

    {
        auto sched4 = std::make_shared<JobWorkStealingScheduler>(4);
        auto pool4 = ThreadPool::create(sched4, 4);

        auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(springForce,
                                                                   [](const Particle &p) {
                                                                       return p.mass;
                                                                   },
                                                                   [](Particle &p) -> Vec3f& {
                                                                       return p.acceleration;
                                                                   }
                                                                   );

        VerletIntegrator<Particle, Vec3f> sim4(pool4, &particles_parallel,
                                               [](Particle &p) -> Vec3f& {
                                                   return p.position;
                                               },
                                               [](Particle &p) -> Vec3f& {
                                                   return p.velocity;
                                               },
                                               [](Particle &p) -> Vec3f& {
                                                   return p.acceleration;
                                               },
                                               accel_calc);

        const float dt = 0.01f;
        for(int i=0; i < 100; ++i)
            sim4.step(dt);

        pool4->shutdown();
    }

    for(size_t i=0; i < particle_count; ++i) {
        // pos should be identical
        REQUIRE_THAT(particles_serial[i].position.x, Catch::Matchers::WithinRel(particles_parallel[i].position.x, 0.0001f));
        REQUIRE_THAT(particles_serial[i].position.y, Catch::Matchers::WithinRel(particles_parallel[i].position.y, 0.0001f));

        // Velocities should be identical
        REQUIRE_THAT(particles_serial[i].velocity.x, Catch::Matchers::WithinRel(particles_parallel[i].velocity.x, 0.0001f));
    }
}

TEST_CASE("VerletIntegrator handles Damped Forces (Energy Decay)", "[threading][science][verlet][damping]")
{
    auto scheduler = std::make_shared<JobWorkStealingScheduler>(4);
    auto pool = ThreadPool::create(scheduler, 4);

    std::vector<Particle> particles;
    Particle p;
    p.id = 1;
    // Stretched spring
    p.position = {10.0f, 0.0f, 0.0f};
    p.velocity = {0.0f, 0.0f, 0.0f};
    p.mass = 1.0f;
    particles.push_back(p);

    auto accel_calc = makeForceToAccelAdapter<Particle, Vec3f>(dampedSpringForce,
                                                               [](const Particle &p) {
                                                                   return p.mass;
                                                               },
                                                               [](Particle &p) -> Vec3f& {
                                                                   return p.acceleration;
                                                               }
                                                               );

    using Integrator = VerletIntegrator<Particle, Vec3f>;
    Integrator sim(pool, &particles,
                   [](Particle &p) -> Vec3f& {
                       return p.position;
                   },
                   [](Particle &p) -> Vec3f& {
                       return p.velocity;
                   },
                   [](Particle &p) -> Vec3f& {
                       return p.acceleration;
                   },
                   accel_calc);

    // Calculate initial energy (k=1.0)
    double E_initial = calculateTotalEnergy(particles, 1.0f);
    REQUIRE(E_initial > 0.0);

    const float dt = 0.05f;
    const int steps = 500;

    for(int i=0; i < steps; ++i)
        sim.step(dt);

    double E_final = calculateTotalEnergy(particles, 1.0f);

    JOB_LOG_INFO("[Damped Energy] Initial: {}, Final: {}", E_initial, E_final);

    REQUIRE(E_final < E_initial);

    // Should be close to zero after 500 steps with c=0.5 damping
    REQUIRE_THAT(E_final, Catch::Matchers::WithinAbs(0.0, 0.5));

    pool->shutdown();
}

//CHECKPOINT v1.1
