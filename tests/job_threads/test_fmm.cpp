#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "job_work_stealing_scheduler.h"
#include "test_thread_science.h"
#include <utils/job_fmm_integrator.h>
#include <cmath>

using namespace job::threads;

// Old lets let this burn up in a .....
// struct TestBody {
//     Vec3f pos;
//     float mass;
//     Vec3f force{0,0,0};
// };

struct TestTraits {
    static Vec3f position(const Particle &particle)
    {
        return particle.position;
    }
    static float mass(const Particle &particle)
    {
        return particle.mass;
    }
    static void applyForce(Particle &particle, const Vec3f &acceleration)
    {
        particle.acceleration = particle.acceleration + acceleration;
    }
};

using FMMEngine = JobFmmEngine<Particle, Vec3f, float, TestTraits>;

// Brute force N^2 to get the "Ground Truth"
void computeExactForces(std::vector<Particle> &particles)
{
    for(size_t i = 0; i < particles.size(); ++i) {
        particles[i].acceleration = {0,0,0};
        for(size_t j = 0; j < particles.size(); ++j) {
            if(i==j)
                continue;

            auto r = particles[j].position - particles[i].position;
            float distSq = r.x*r.x + r.y*r.y + r.z*r.z;
            float invDist = 1.0f/std::sqrt(distSq);
            float invDist3 = invDist * invDist * invDist;
            // F = G * m1 * m2 * r / r^3
            auto f = r * (particles[j].mass * invDist3); // Mass of *other*
            particles[i].acceleration = particles[i].acceleration + f;
        }
    }
}

static Vec3f exactForceOnTarget(const std::vector<Particle> &sources,
                                const Particle &target)
{
    Vec3f exact{0,0,0};
    for (const auto &s : sources) {
        Vec3f dr = s.position - target.position;   // source - target (same as P2P)
        float r2 = dr.x*dr.x + dr.y*dr.y + dr.z*dr.z;
        if (r2 <= 1e-10f)
            continue;               // just in case
        float invR  = 1.0f / std::sqrt(r2);
        float invR3 = invR * invR * invR;
        exact = exact + dr * (s.mass * invR3);
    }
    return exact;
}


// This all needs to be in the Utils or something of the lib top make this API much more simple
TEST_CASE("FMM Kernel Integrity (P=3 Octupole)", "[fmm][kernel][math]")
{
    using Kernel = FmmKernels<Vec3f, float>;
    using Coeffs = FmmCoefficients<Vec3f, float>;

    // Setup Geometry
    // Cluster: Two particles at +/- 1.0 (Size ~ 2.0)
    // Target:  Point at +20.0 (Dist = 20.0)
    // Ratio:   1/20 = 0.05.
    // Expected P=3 Error: (0.05)^4 = 6e-6. VERY SMALL.

    Vec3f center{0,0,0};
    std::vector<Particle> sources = {
        {.position{-1.0f, 0.0f, 0.0f}, .mass = 1.0f},
        {.position{ 1.0f, 0.0f, 0.0f}, .mass = 1.0f}
    };

    Particle target{ .position{0.0f, 20.0f, 0.0f}, .mass = 1.0f };

    Vec3f exactForce = exactForceOnTarget(sources, target);

    Coeffs M;
    // you go to the center damit, I hate that I am doing this here
    for(const auto &s : sources)
        Kernel::P2M(M, s.position, s.mass, center);


    // monopole: 2.0
    CHECK(M.monopole == 2.0f);

    // dipole: 0 (Symmetric)
    CHECK(std::abs(M.dipole.x) < 1e-5);

    // quadrupole Qxx: m*x^2 = 1*(-1)^2 + 1*(1)^2 = 2.0
    CHECK(std::abs(M.q_xx - 2.0f) < 1e-5);

    Coeffs L;
    // shiftVec = TargetCenter - SourceCenter
    Vec3f shift = target.position - center;
    Kernel::M2L(L, M, shift);

    Kernel::template L2P<Particle, TestTraits>(target, L, target.position);

    float errY = std::abs(target.acceleration.y - exactForce.y);
    float relError = errY / std::abs(exactForce.y);

    JOB_LOG_INFO("[Fmm Integrity] Exact Force Y: {}",  exactForce.y);
    JOB_LOG_INFO("[Fmm Integrity] FMM Force Y:   {}",  target.acceleration.y);
    JOB_LOG_INFO("[Fmm Integrity] Rel Error:     {}",  relError);

    // Expectation:
    // With d/R = 0.05, P=3 should be nearly perfect float precision.
    // Float precision is ~1e-7.
    // Truncation error is ~6e-6.
    // We should easily pass 0.01% (1e-4).

    REQUIRE_THAT(relError, Catch::Matchers::WithinAbs(0.0f, 0.0001f)); // bad now that I tried to make into Particle
}

TEST_CASE("FMM Kernel Integrity (P2M -> M2L)", "[fmm][kernel]")
{
    using Kernel = FmmKernels<Vec3f, float>;
    using Coeffs = FmmCoefficients<Vec3f, float>;

    Coeffs M;
    Vec3f center{0,0,0};

    Kernel::P2M(M, {-1.0f, 0.0f, 0.0f}, 1.0f, center);
    Kernel::P2M(M, { 1.0f, 0.0f, 0.0f}, 1.0f, center);

    REQUIRE(M.monopole == 2.0f);
    REQUIRE(M.dipole.x == 0.0f);
    REQUIRE(M.q_xx == 2.0f);
    REQUIRE(M.q_yy == 0.0f);

    Coeffs L;
    Vec3f targetPos{0.0f, 10.0f, 0.0f};
    Vec3f r = targetPos - center;

    Kernel::M2L(L, M, r);
    float pointMassForce = -0.02f;

    // Quadrupole The "dumbell" is along X(hehe). We are at Y(hehe).

    float fmmForce = -L.dipole.y;

    INFO("Point Mass Force: " << pointMassForce);
    INFO("FMM Calc Force: " << fmmForce);

    // Just ensure it calculated *something* different than pure monopole
    REQUIRE(std::abs(fmmForce - pointMassForce) > 1e-6);
}

TEST_CASE("FMM Quadrupole Accuracy (Regresion 0.7%)", "[fmm][math][quadrupole]")
{
    auto sched = std::make_shared<JobWorkStealingScheduler>(4);
    auto pool = ThreadPool::create(sched, 4);
    std::vector<Particle> bodies {
         { .position{-2.0f, 0.0f, 0.0f},  .mass = 1.0f },
         { .position{ 2.0f, 0.0f, 0.0f},  .mass = 1.0f },
         { .position{ 0.0f, 20.0f, 0.0f}, .mass = 1.0f}
    };

    computeExactForces(bodies);
    float exactForceY = bodies[2].acceleration.y;

    bodies[2].acceleration = {0,0,0};

    JobFmmEngine<Particle, Vec3f, float, TestTraits>::Params params;
    // (M2L)
    params.maxLeafSize = 1;
    params.theta = 0.9f;

    JobFmmEngine<Particle, Vec3f, float, TestTraits> engine(pool, params);
    engine.compute(bodies);

    float fmmForceY = bodies[2].acceleration.y;

    // The "Error"
    float error = std::abs(fmmForceY - exactForceY);
    float relError = error / std::abs(exactForceY);

    JOB_LOG_INFO("[Fmm Regression] Ground Truth Force Y: {}", exactForceY);
    JOB_LOG_INFO("[Fmm Regression] FMM Quadrupole Force Y: {}", fmmForceY);
    JOB_LOG_INFO("[Fmm Regression] Relative Error: {}", relError);

    REQUIRE(relError < 0.07f);
    pool->shutdown();
}


TEST_CASE("FMM translation invariance (P=3)", "[fmm][invariance]")
{
    auto sched = std::make_shared<JobWorkStealingScheduler>(4);
    auto pool  = ThreadPool::create(sched, 4);

    FMMEngine::Params params;
    params.maxLeafSize = 4;
    params.theta = 0.6f;

    // Random-ish small system
    std::vector<Particle> orig {
        { .position{-2.0f, 1.0f,  0.5f}, .mass = 1.0f },
        { .position{ 1.5f,-1.0f, -0.2f}, .mass = 2.0f },
        { .position{ 0.5f, 3.0f,  1.0f}, .mass = 0.5f },
        { .position{-1.0f,-2.0f,  0.0f}, .mass = 1.2f }
    };

    Vec3f shift{10.0f, -3.5f, 2.0f};
    std::vector<Particle> shifted = orig;
    for (auto &p : shifted)
        p.position = p.position + shift;


    FMMEngine engine(pool, params);

    // Compute forces for original
    engine.compute(orig);

    // Compute forces for shifted system
    engine.compute(shifted);

    for (size_t i = 0; i < orig.size(); ++i) {
        auto a0 = orig[i].acceleration;
        auto a1 = shifted[i].acceleration;

        REQUIRE(std::abs(a0.x - a1.x) < 1e-4f);
        REQUIRE(std::abs(a0.y - a1.y) < 1e-4f);
        REQUIRE(std::abs(a0.z - a1.z) < 1e-4f);
    }

    pool->shutdown();
}


