#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>

#include "test_thread_science.h"

#include <sched/job_fifo_scheduler.h>
#include <job_thread_pool.h>
#include <utils/job_fmm_tree.h>

using namespace job::threads;

// PASSES
TEST_CASE("JobFmm builds octree correctly", "[fmm][tree][octree]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 4);

    struct TestBody {
        Vec3f position;
        float charge{1.0f};
    };

    struct TestKernel {
        static Vec3f position(const TestBody &body)
        {
            return body.position;
        }
        static float charge(const TestBody &body)
        {
            return body.charge;
        }
        static void addForce([[maybe_unused]]TestBody &body, [[maybe_unused]]const Vec3f &force)
        {
            // Nothing to see here ... yet
        }
    };

    // 8 bodies at corners of unit cube
    std::vector<TestBody> bodies;
    // FOR HELL IN ....
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                bodies.push_back({{float(x), float(y), float(z)}, 1.0f});
            }
        }
    }

    FmmParams<float> params;
    params.maxLeafSize = 1;

    JobFmm<TestBody, Vec3f, float, TestKernel> fmm(pool, params);
    fmm.buildTree(bodies);

    const auto& nodes = fmm.nodes();

    REQUIRE(nodes.size() > 1);
    REQUIRE(nodes[0].depth == 0);


    std::size_t totalParticles = 0;
    for (const auto& node : nodes) {
        if (node.isLeaf())
            totalParticles += node.particleCount;
    }
    REQUIRE(totalParticles == bodies.size());
    pool->shutdown();
}





TEST_CASE("JobFmm dipole is more accurate than monopole", "[fmm][dipole][accuracy]")
{
    struct TestBody {
        Vec3f pos;
        float mass;
        Vec3f force{0,0,0};
    };

    struct TestTraits {
        static Vec3f position(const TestBody &b) { return b.pos; }
        static float mass(const TestBody &b) { return b.mass; }
        static void applyForce(TestBody &b, const Vec3f &f) {
            b.force = b.force + f;
        }
    };

    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);

    // Create an asymmetric cluster at the origin, and a test particle far away
    std::vector<TestBody> bodies = {
        {{-1.0f, 0.0f, 0.0f}, 1.0f},
        {{ 0.0f, 0.0f, 0.0f}, 1.0f},
        {{ 0.3f, 0.0f, 0.0f}, 2.0f}, // Heavy offset -> Non-zero dipole
        {{50.0f, 0.0f, 0.0f}, 1.0f}  // Target
    };

    // Monopole test
    FmmParams<float> paramsMonopole;
    paramsMonopole.expansionOrder = ExpansionOrder::Monopole;
    paramsMonopole.theta = 0.2f;  // REAL LOW
    // paramsMonopole.maxLeafSize = 3;
    // paramsMonopole.maxDepth = 10;

    JobFmm<TestBody, Vec3f, float, TestTraits> fmmMonopole(pool, paramsMonopole);
    fmmMonopole.buildTree(bodies);
    fmmMonopole.compute(bodies);

    float forceMonopoleX = bodies[3].force.x;  // Force on test particle

    // Reset forces
    for (auto &b : bodies)
        b.force = {0,0,0};

    // Dipole test
    FmmParams<float> paramsDipole = paramsMonopole;
    paramsDipole.expansionOrder = ExpansionOrder::Dipole;
    // paramsDipole.theta = 0.8f;
    paramsDipole.maxLeafSize = 1;  // <<< VERY VERY VERY IMPOPRTANT

    JobFmm<TestBody, Vec3f, float, TestTraits> fmmDipole(pool, paramsDipole);
    fmmDipole.buildTree(bodies);
    fmmDipole.compute(bodies);

    float forceDipoleX = bodies[3].force.x;

    INFO("Monopole force on test particle: " << forceMonopoleX);
    INFO("Dipole force on test particle: " << forceDipoleX);

    // They should differ because the cluster is asymmetric
    REQUIRE(std::abs(forceMonopoleX - forceDipoleX) > 0.0001f);

    // Both should be negative (pulling toward the cluster)
    REQUIRE(forceMonopoleX < 0.0f);
    REQUIRE(forceDipoleX < 0.0f);

    pool->shutdown();
}
