#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <cmath>

#include "test_thread_science.h"

#include <job_thread_pool.h>
#include <sched/job_sporadic_scheduler.h>
#include <utils/job_barnes_hut_calculator.h>

using namespace job::threads;
using namespace std::chrono_literals;

static float vec_rel_error(const Vec3f &a, const Vec3f &b)
{
    float ax2 = a.lengthSq();
    float bx2 = b.lengthSq();
    float denom = std::max({1.0f, std::sqrt(ax2), std::sqrt(bx2)});
    Vec3f diff = a - b;
    float num = diff.length();

    return num / denom;
}

// Direct N/\2 gravitational force on particle i (softened)
static Vec3f direct_force_on(const std::vector<Particle> &ps,
                             std::size_t i, float G, float eps2)
{
    const Particle &pi = ps[i];
    Vec3f F{0.0f, 0.0f, 0.0f};

    for (std::size_t j = 0; j < ps.size(); ++j) {
        if (j == i)
            continue;

        const Particle &pj = ps[j];

        Vec3f r = pj.position - pi.position;
        float r2 = r.lengthSq() + eps2;
        float inv_r = 1.0f / std::sqrt(r2);
        float inv_r3 = inv_r / r2;
        float coeff = G * pi.mass * pj.mass * inv_r3;

        F = F + (r * coeff);
    }
    return F;
}

TEST_CASE("Barnes-Hut matches direct force for 2-body with tiny theta", "[barnes_hut][gravity]")
{
    auto sched = std::make_shared<JobSporadicScheduler>();
    auto pool  = ThreadPool::create(sched, 1, JobThreadOptions::normal());
    REQUIRE(pool != nullptr);

    std::vector<Particle> ps(2);

    ps[0].id        = 1;
    ps[0].position  = Vec3f{-1.0f, 0.0f, 0.0f};
    ps[0].mass      = 1.0f;
    // Particle 2
    ps[1].id        = 2;
    ps[1].position  = Vec3f{ 1.0f, 0.0f, 0.0f};
    ps[1].mass      = 2.0f;

    auto get_pos = [](const Particle &p) -> const Vec3f& { return p.position; };
    auto get_mass = [](const Particle &p) -> float { return p.mass; };

    const float G = 1.0f;
    const float eps2 = 1e-6f;
    const float theta = 1e-6f;  // so small

    BarnesHutForceCalculator<Particle, Vec3f, float> bh(pool, get_pos, get_mass, theta, G, eps2);

    std::vector<Vec3f> F_bh;
    bh.calculate_forces(ps, F_bh);
    pool->shutdown();

    REQUIRE(F_bh.size() == ps.size());

    Vec3f F0_direct = direct_force_on(ps, 0, G, eps2);
    Vec3f F1_direct = direct_force_on(ps, 1, G, eps2);

    REQUIRE(vec_rel_error(F_bh[0], F0_direct) < 1e-4f);
    REQUIRE(vec_rel_error(F_bh[1], F1_direct) < 1e-4f);

    Vec3f Fsum      = F_bh[0] + F_bh[1];
    float sum_norm  = Fsum.length();
    float scale     = std::sqrt(F_bh[0].lengthSq() + F_bh[1].lengthSq());
    REQUIRE(sum_norm < 1e-4f * std::max(1.0f, scale));
}

TEST_CASE("Barnes-Hut approximates direct forces for small cluster", "[barnes_hut][gravity]")
{
    auto sched = std::make_shared<JobSporadicScheduler>();
    auto pool  = ThreadPool::create(sched, /*threadCount=*/2,
                                   JobThreadOptions::normal());
    REQUIRE(pool != nullptr);

    std::vector<Particle> ps;
    ps.reserve(8);

    ps.push_back(Particle{
        .id = 1,
        .position = Vec3f{-1.0f, 0.2f, 0.1f},
        .radius = 1.0f,
        .mass = 1.0f
    });
    ps.push_back(Particle{
        .id = 2,
        .position = Vec3f{ 0.5f, -0.3f, 0.4f},
        .radius = 1.0f,
        .mass = 2.0f
    });
    ps.push_back(Particle{
        .id = 3,
        .position = Vec3f{ 1.2f, 0.8f, 0.0f},
        .radius = 1.0f,
        .mass = 1.5f
    });
    ps.push_back(Particle{
        .id = 4,
        .position = Vec3f{-0.7f, -1.1f, 0.2f},
        .radius = 1.0f,
        .mass = 0.8f
    });
    ps.push_back(Particle{
        .id = 5,
        .position = Vec3f{ 0.0f, 0.0f, 1.0f},
        .radius = 1.0f,
        .mass = 3.0f
    });

    auto get_pos = [](const Particle &p) -> const Vec3f& { return p.position; };
    auto get_mass = [](const Particle &p) -> float{ return p.mass; };

    const float G     = 1.0f;
    const float eps2  = 1e-4f;
    const float theta = 0.5f;

    BarnesHutForceCalculator<Particle, Vec3f, float> bh(pool, get_pos, get_mass, theta, G, eps2);

    std::vector<Vec3f> F_bh;
    bh.calculate_forces(ps, F_bh);
    pool->shutdown();

    REQUIRE(F_bh.size() == ps.size());

    for (std::size_t i = 0; i < ps.size(); ++i) {
        Vec3f F_direct = direct_force_on(ps, i, G, eps2);
        float err = vec_rel_error(F_bh[i], F_direct);

        INFO("particle i=" << i << " rel error=" << err);
        REQUIRE(err < 0.05f);
    }
}


TEST_CASE("Barnes-Hut: empty and single-particle are safe", "[barnes_hut][edge]")
{
    auto sched = std::make_shared<JobSporadicScheduler>();
    auto pool  = ThreadPool::create(sched, 1, JobThreadOptions::normal());
    REQUIRE(pool != nullptr);

    auto get_pos = [](const Particle &p) -> const Vec3f& { return p.position; };
    auto get_mass = [](const Particle &p) -> float { return p.mass; };

    BarnesHutForceCalculator<Particle, Vec3f, float> bh(pool, get_pos, get_mass, 0.5f, 1.0f, 1e-4f);

    std::vector<Particle> ps;
    std::vector<Vec3f> F;
    bh.calculate_forces(ps, F);
    REQUIRE(F.empty());

    ps.push_back(Particle{ .id=1, .position=Vec3f{0.0f,0.0f,0.0f}, .radius=1.0f, .mass=1.0f });
    bh.calculate_forces(ps, F);
    REQUIRE(F.size() == 1);
    REQUIRE(F[0].lengthSq() == Catch::Approx(0.0f).margin(1e-8f));
}

TEST_CASE("Barnes-Hut approximates direct forces for small cluster", "[barnes_hut][gravity][momentum]")
{
    auto sched = std::make_shared<JobSporadicScheduler>();
    auto pool  = ThreadPool::create(sched, /*threadCount=*/2,
                                   JobThreadOptions::normal());
    REQUIRE(pool != nullptr);

    std::vector<Particle> ps;
    ps.reserve(8);

    // A small, non-symmetric 3D configuration
    ps.push_back(Particle{
        .id = 1,
        .position = Vec3f{-1.0f, 0.2f, 0.1f},
        .radius = 1.0f,
        .mass = 1.0f
    });
    ps.push_back(Particle{
        .id = 2,
        .position = Vec3f{ 0.5f, -0.3f, 0.4f},
        .radius = 1.0f,
        .mass = 2.0f
    });
    ps.push_back(Particle{
        .id = 3,
        .position = Vec3f{ 1.2f, 0.8f, 0.0f},
        .radius = 1.0f,
        .mass = 1.5f
    });
    ps.push_back(Particle{
        .id = 4,
        .position = Vec3f{-0.7f, -1.1f, 0.2f},
        .radius = 1.0f,
        .mass = 0.8f
    });
    ps.push_back(Particle{
        .id = 5,
        .position = Vec3f{ 0.0f, 0.0f, 1.0f},
        .radius = 1.0f,
        .mass = 3.0f
    });

    auto get_pos  = [](const Particle& p) -> const Vec3f& { return p.position; };
    auto get_mass = [](const Particle& p) -> float       { return p.mass; };

    const float G     = 1.0f;
    const float eps2  = 1e-4f;
    const float theta = 0.5f;

    BarnesHutForceCalculator<Particle, Vec3f, float> bh(pool, get_pos, get_mass,
                                                        theta, G, eps2);

    std::vector<Vec3f> F_bh;
    bh.calculate_forces(ps, F_bh);
    pool->shutdown();

    REQUIRE(F_bh.size() == ps.size());

    Vec3f sum_bh{0,0,0};
    Vec3f sum_direct{0,0,0};
    float force_scale = 0.0f;

    for (std::size_t i = 0; i < ps.size(); ++i) {
        Vec3f F_direct = direct_force_on(ps, i, G, eps2);
        float err = vec_rel_error(F_bh[i], F_direct);

        INFO("particle i=" << i << " rel error=" << err);
        REQUIRE(err < 0.05f);

        sum_bh = sum_bh + F_bh[i];
        sum_direct = sum_direct + F_direct;
        force_scale += F_direct.lengthSq();
    }
    force_scale = std::sqrt(force_scale); // Get the scale of total forces

    INFO("Direct Sum: " << sum_direct.x << ", " << sum_direct.y << ", " << sum_direct.z);
    INFO("BH Sum: " << sum_bh.x << ", " << sum_bh.y << ", " << sum_bh.z);

    REQUIRE(sum_direct.length() < 1e-4f * std::max(1.0f, force_scale));
    REQUIRE(vec_rel_error(sum_bh, sum_direct) < 1e-2f);
}
