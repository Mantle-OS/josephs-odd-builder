#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <cmath>

// test data

#include <job_logger.h>

#include <job_thread_pool.h>
#include <job_stealing_ctx.h>

#include <data/vec3f.h>
#include <data/particle.h>
#include <data/forces.h>

#include <solvers/job_verlet_adapters.h>
#include <solvers/job_verlet_integrator.h>


using namespace job::threads;
using namespace job::science::data;
using namespace std::chrono_literals;

using Integrator = job::science::VerletIntegrator<Particle, Vec3f>;
using VV_Scheme = job::science::VVScheme;
TEST_CASE("VerletIntegrator runs KDK and DKD with generic (POD) structs", "[threading][science][verlet]")
{

    auto sched = JobStealerCtx(4);

    // Two particle's lists to compare KDK vs DKD
    Particles particles_kdk;
    Particles particles_dkd;

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
    // adapt "springForce" function into the AccelCalculator the integrator expects.
    ///////////////////////////////////////////////////////////////////////////
    auto accel_calc = job::science::makeForceToAccelAdapter<Particle, Vec3f>(Forces::springForce,
                                                               [](const Particle &p) { return p.mass; },
                                                               [](Particle &p) -> Vec3f& { return p.acceleration; }
                                                               );


    Integrator sim_kdk(sched.pool, &particles_kdk,
                       [](Particle &p) -> Vec3f& { return p.position; },
                       [](Particle &p) -> Vec3f& { return p.velocity; },
                       [](Particle &p) -> Vec3f& { return p.acceleration; },
                       accel_calc
                       );

    Integrator sim_dkd(sched.pool, &particles_dkd,
                       [](Particle &p) -> Vec3f& { return p.position; },
                       [](Particle &p) -> Vec3f& { return p.velocity; },
                       [](Particle &p) -> Vec3f& { return p.acceleration; },
                       accel_calc
                       );


    const float dt = 0.01f;
    const int steps = 10000;

    JOB_LOG_INFO("[VELOCITY VERLET]: SCIENCE TIME Running {} loops on the simulation", steps);
    for (int i = 0; i < steps; ++i) {
        sim_kdk.step(dt, VV_Scheme::KDK, true);
        sim_dkd.step(dt, VV_Scheme::DKD, true);
    }


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
    auto sched = JobStealerCtx(1);

    Particles particles;
    Particle p;
    p.id = 1;
    p.position = {10.0f, 0.0f, 0.0f}; // Displaced
    p.velocity = {0.0f, 5.0f, 0.0f};  // Initial velocity
    p.acceleration = {0.0f, 0.0f, 0.0f};
    p.mass = 2.0f;
    particles.push_back(p);

    // Spring Constant from springForce in test_thread_science.h is 1.0f
    const float k_spring = 1.0f;

    auto accel_calc = job::science::makeForceToAccelAdapter<Particle, Vec3f>(Forces::springForce,
                                                               [](const Particle &p) { return p.mass; },
                                                               [](Particle &p) -> Vec3f& { return p.acceleration; }
                                                               );

    Integrator sim(sched.pool, &particles,
                   [](Particle &p) -> Vec3f& { return p.position; },
                   [](Particle &p) -> Vec3f& { return p.velocity; },
                   [](Particle &p) -> Vec3f& { return p.acceleration; },
                   accel_calc
                   );

    double initial_energy = Forces::calculateTotalEnergy(particles, k_spring);
    const float dt = 0.01f;
    const int steps = 5000;

    for(int i=0; i < steps; ++i)
        sim.step(dt, job::science::VVScheme::KDK, true);


    double final_energy = Forces::calculateTotalEnergy(particles, k_spring);

    JOB_LOG_INFO("[Verlet Energy] Initial: {}, Final: {}", initial_energy, final_energy);

    // Symplectic integrators should keep energy bounded. It will oscillate slightly but shouldn't drift significantly like Bueler ... Bueler .... Euler.
    // We allow a small epsilon for floating point accumulation over 5000 steps.
    REQUIRE_THAT(final_energy, Catch::Matchers::WithinRel(initial_energy, 0.001)); // 0.1% tolerance

}

TEST_CASE("VerletIntegrator Determinism: Single Thread vs Thread Pool", "[threading][science][verlet][concurrency]")
{
    const size_t particle_count = 1000;

    Particles particles_serial(particle_count);
    Particles particles_parallel(particle_count);

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
        auto singleThread = JobStealerCtx(1);
        auto accel_calc = job::science::makeForceToAccelAdapter<Particle, Vec3f>(Forces::springForce,
                                                                   [](const Particle &p) { return p.mass; },
                                                                   [](Particle &p) -> Vec3f& { return p.acceleration; }
                                                                   );

        Integrator sim1(singleThread.pool, &particles_serial,
                                               [](Particle &p) -> Vec3f& { return p.position; },
                                               [](Particle &p) -> Vec3f& { return p.velocity; },
                                               [](Particle &p) -> Vec3f& { return p.acceleration; },
                                               accel_calc);

        const float dt = 0.01f;
        for(int i=0; i<100; ++i)
            sim1.step(dt);
    }

    {
        auto fourThreads = JobStealerCtx(4);
        auto accel_calc = job::science::makeForceToAccelAdapter<Particle, Vec3f>(Forces::springForce,
                                                                   [](const Particle &p) { return p.mass; },
                                                                   [](Particle &p) -> Vec3f& { return p.acceleration; }
                                                                   );

        Integrator sim4(fourThreads.pool, &particles_parallel,
                                               [](Particle &p) -> Vec3f& { return p.position; },
                                               [](Particle &p) -> Vec3f& { return p.velocity; },
                                               [](Particle &p) -> Vec3f& { return p.acceleration; },
                                               accel_calc);

        const float dt = 0.01f;
        for(int i=0; i < 100; ++i)
            sim4.step(dt);
    }

    for(size_t i=0; i < particle_count; ++i) {
        // what a POS ...  should be identical
        REQUIRE_THAT(particles_serial[i].position.x, Catch::Matchers::WithinRel(particles_parallel[i].position.x, 0.0001f));
        REQUIRE_THAT(particles_serial[i].position.y, Catch::Matchers::WithinRel(particles_parallel[i].position.y, 0.0001f));

        // More twins. Velocities should be identical
        REQUIRE_THAT(particles_serial[i].velocity.x, Catch::Matchers::WithinRel(particles_parallel[i].velocity.x, 0.0001f));
    }
}

TEST_CASE("VerletIntegrator handles Damped Forces (Energy Decay)", "[threading][science][verlet][damping]")
{
    auto sched = JobStealerCtx(4);
    Particles particles;
    Particle p;
    p.id = 1;
    // Stretch(armstrong)ed spring
    p.position = {10.0f, 0.0f, 0.0f};
    p.velocity = {0.0f, 0.0f, 0.0f};
    p.mass = 1.0f;
    particles.push_back(p);

    auto accel_calc = job::science::makeForceToAccelAdapter<Particle, Vec3f>(Forces::dampedSpringForce,
                                                               [](const Particle &p) { return p.mass; },
                                                               [](Particle &p) -> Vec3f& { return p.acceleration; }
                                                               );

    Integrator sim(sched.pool, &particles,
                   [](Particle &p) -> Vec3f& { return p.position; },
                   [](Particle &p) -> Vec3f& { return p.velocity; },
                   [](Particle &p) -> Vec3f& { return p.acceleration; },
                   accel_calc);

    // Calculate initial energy (k=1.0)
    double E_initial = Forces::calculateTotalEnergy(particles, 1.0f);
    REQUIRE(E_initial > 0.0);

    const float dt = 0.05f;
    const int steps = 500;

    for(int i=0; i < steps; ++i)
        sim.step(dt);

    double E_final = Forces::calculateTotalEnergy(particles, 1.0f);

    JOB_LOG_INFO("[Damped Energy] Initial: {}, Final: {}", E_initial, E_final);

    REQUIRE(E_final < E_initial);

    // Your a HERO and a ZERO
    // Should be close to zero after 500 steps with c=0.5 damping
    REQUIRE_THAT(E_final, Catch::Matchers::WithinAbs(0.0, 0.5));
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Verlet Integrator: Step throughput baseline (serial vs threaded)", "[threading][science][verlet][bench]")
{
    constexpr size_t bench = 125'000;
    constexpr size_t benchDouble = 250'000;
    constexpr float dt = 0.01f;

    auto singleThread = JobStealerCtx(1);
    auto fourThreads = JobStealerCtx(4);

    auto particlesSerial = ParticleUtil::genParticles(bench);
    auto particlesThread = ParticleUtil::genParticles(bench);

    auto particlesDoubleSerial = ParticleUtil::genParticles(benchDouble);
    auto particlesDoubleThread = ParticleUtil::genParticles(benchDouble);

    auto accelSerial = job::science::makeForceToAccelAdapter<Particle, Vec3f>(
        Forces::expensiveSpringForce,
        [](const Particle &p) { return p.mass; },
        [](Particle &p) -> Vec3f& { return p.acceleration; }
        );


    auto accelThreaded = job::science::makeForceToAccelAdapter<Particle, Vec3f>(
        Forces::expensiveSpringForce,
        [](const Particle &p) { return p.mass; },
        [](Particle &p) -> Vec3f& { return p.acceleration; }
        );


    auto accelDoubleSerial = job::science::makeForceToAccelAdapter<Particle, Vec3f>(
        Forces::expensiveSpringForce,
        [](const Particle &p) { return p.mass; },
        [](Particle &p) -> Vec3f& { return p.acceleration; }
        );

    auto accelDoubleThreaded = job::science::makeForceToAccelAdapter<Particle, Vec3f>(
        Forces::expensiveSpringForce,
        [](const Particle &p) { return p.mass; },
        [](Particle &p) -> Vec3f& { return p.acceleration; }
        );

    Integrator simSerial(singleThread.pool, &particlesSerial,
                          [](Particle &p) -> Vec3f& { return p.position; },
                          [](Particle &p) -> Vec3f& { return p.velocity; },
                          [](Particle &p) -> Vec3f& { return p.acceleration; },
                          accelSerial
                          );

    Integrator simThreaded(fourThreads.pool, &particlesThread,
                            [](Particle &p) -> Vec3f& { return p.position; },
                            [](Particle &p) -> Vec3f& { return p.velocity; },
                            [](Particle &p) -> Vec3f& { return p.acceleration; },
                            accelThreaded
                            );


    Integrator simDoubleSerial(singleThread.pool, &particlesDoubleSerial,
                         [](Particle &p) -> Vec3f& { return p.position; },
                         [](Particle &p) -> Vec3f& { return p.velocity; },
                         [](Particle &p) -> Vec3f& { return p.acceleration; },
                         accelDoubleSerial
                         );

    Integrator simDoubleThreaded(fourThreads.pool, &particlesDoubleThread,
                           [](Particle &p) -> Vec3f& { return p.position; },
                           [](Particle &p) -> Vec3f& { return p.velocity; },
                           [](Particle &p) -> Vec3f& { return p.acceleration; },
                           accelThreaded
                           );


    for (int i = 0; i < 5; ++i) {
        simSerial.step(dt, job::science::VVScheme::KDK, false);
        simThreaded.step(dt, VV_Scheme::KDK, true);
    }
    for (int i = 0; i < 5; ++i) {
        simDoubleSerial.step(dt, VV_Scheme::KDK, false);
        simDoubleThreaded.step(dt, VV_Scheme::KDK, true);
    }

    //
    BENCHMARK("[Verlet Serial] KDK Step (Serial, N=125k)") {
        simSerial.step(dt, VV_Scheme::KDK, false);
    };
    BENCHMARK("[Verlet Scale Serial] KDK Step (Serial, N=250k)") {
        simDoubleSerial.step(dt, VV_Scheme::KDK, false);
    };
    //
    BENCHMARK("[Verlet Thread] KDK Step (Threaded, N=125K)") {
        simThreaded.step(dt, VV_Scheme::KDK, true);
    };
    BENCHMARK("[Verlet Scale Thread] KDK Step (Threaded, N=250k)") {
        simDoubleThreaded.step(dt, VV_Scheme::KDK, true);
    };
    //
    BENCHMARK("[Verlet Serial] DKD Step (Serial, N=125k)") {
        simSerial.step(dt, VV_Scheme::DKD, false);
    };
    BENCHMARK("[Verlet Scale Serial] DKD Step (Serial, N=250k)") {
        simDoubleSerial.step(dt, VV_Scheme::DKD, false);
    };
    //
    BENCHMARK("[Verlet Thread] DKD Step (Threaded, N=125k)") {
        simThreaded.step(dt, VV_Scheme::DKD, true);
    };
    BENCHMARK("[Verlet Scale Thread] DKD Step (Threaded, N=250k)") {
        simDoubleThreaded.step(dt, VV_Scheme::DKD, true);
    };

}

#endif // JOB_TEST_BENCHMARKS


