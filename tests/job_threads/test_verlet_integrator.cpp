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

// simple harmonic oscillator (F = -kx)
// We use the "ForceFn" signature: void(particles, out_forces)
void SimpleSpringForce(const std::vector<Particle>& particles, std::vector<Vec3f>& out_forces)
{

    const float sprint_const = 1.0f;
    // F = -sprint_const * x
    for (size_t i = 0; i < particles.size(); ++i)
        out_forces[i] = particles[i].position * -sprint_const;
}


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
    auto accel_calc = make_force_to_accel_adapter<Particle, Vec3f>(
        SimpleSpringForce,
        [](const Particle &p) { return p.mass; },         // GetMass
        [](Particle &p) -> Vec3f& { return p.acceleration; } // GetAcc
        );

    // Integrators
    using Integrator = VerletIntegrator<Particle, Vec3f>;

    Integrator sim_kdk(
        pool, &particles_kdk,
        [](Particle &p) -> Vec3f& { return p.position; },
        [](Particle &p) -> Vec3f& { return p.velocity; },
        [](Particle &p) -> Vec3f& { return p.acceleration; },
        accel_calc
        );

    Integrator sim_dkd(
        pool, &particles_dkd,
        [](Particle &p) -> Vec3f& { return p.position; },
        [](Particle &p) -> Vec3f& { return p.velocity; },
        [](Particle &p) -> Vec3f& { return p.acceleration; },
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


