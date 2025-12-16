#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Include benchmark header if compiled with benchmarks enabled
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_stealing_ctx.h>

// The New Architecture
#include <es_coach.h>
#include <learn_type.h>
#include <layer_factory.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;
using namespace job::ai::learn;
using namespace job::threads;

// -----------------------------------------------------------------------------
// HELPER: Build XOR Genome
// -----------------------------------------------------------------------------
Genome buildXORGenome()
{
    Genome g;
    // Layer 1: 2 -> 8 (Tanh is safer for small nets than ReLU)
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = comp::ActivationType::Tanh;
    l1.inputs = 2;
    l1.outputs = 8;
    l1.weightCount = (2 * 8) + 8;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    // Layer 2: 8 -> 1 (Sigmoid)
    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Sigmoid;
    l2.inputs = 8;
    l2.outputs = 1;
    l2.weightCount = (8 * 1) + 1;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f); // Range [-1.0, 1.0]

    return g;
}

// -----------------------------------------------------------------------------
// HELPER: Build CartPole Genome (Robust Init)
// -----------------------------------------------------------------------------
Genome buildCartPoleGenome()
{
    Genome g;
    // Layer 1: 4 Inputs -> 32 Hidden
    // Use Tanh. It behaves linearly near 0, preventing "Dead ReLU" on start.
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = comp::ActivationType::Tanh;
    l1.inputs = 4;
    l1.outputs = 32;
    l1.weightCount = (4 * 32) + 32; // Weights + Biases
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    // Layer 2: 32 Hidden -> 2 Logits
    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Identity;
    l2.inputs = 32;
    l2.outputs = 2;
    l2.weightCount = (32 * 2) + 2;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    // Wider initialization to ensure we don't start with 0 output
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f) * 1.0f; // Range [-1.0, 1.0]

    return g;
}

// -----------------------------------------------------------------------------
// TESTS
// -----------------------------------------------------------------------------

TEST_CASE("ESCoach: Solves XOR", "[ai][coach][es][xor]")
{
    JobStealerCtx ctx(8);

    Genome parent = buildXORGenome();

    ESCoach::Config cfg;
    cfg.populationSize = 128;
    cfg.sigma = 0.1f;
    cfg.decay = 0.99f;
    cfg.envConfig.type = LearnType::XOR;
    cfg.envConfig.initWsMb = 1;

    ESCoach coach(ctx.pool, cfg);

    float bestFit = 0.0f;
    for(int i = 0; i < 100; ++i) {
        coach.coach((i == 0) ? parent : coach.bestGenome());
        bestFit = coach.currentBestFitness();
        if (bestFit > 0.98f) break;
    }

    INFO("Final XOR Fitness: " << bestFit);
    REQUIRE(bestFit > 0.90f);
}

TEST_CASE("ESCoach: Masters CartPole", "[ai][coach][es][cartpole]")
{
    JobStealerCtx ctx(8);

    Genome parent = buildCartPoleGenome();

    ESCoach::Config cfg;
    cfg.populationSize = 128; // Decent population size
    cfg.sigma = 0.2f;         // HIGHER SIGMA to break out of "suicide" mode
    cfg.decay = 0.99f;        // Anneal gently
    cfg.envConfig.type = LearnType::CartPole;
    cfg.envConfig.initWsMb = 1;

    ESCoach coach(ctx.pool, cfg);

    float bestFit = 0.0f;
    int solvedGen = -1;

    JOB_LOG_INFO("CartPole Starteed");
    for(int i = 0; i < 100; ++i) { // Give it 100 gens
        coach.coach((i == 0) ? parent : coach.bestGenome());

        bestFit = coach.currentBestFitness();

        if (bestFit >= 490.0f) {
            solvedGen = i;
            break;
        }
    }

    if (solvedGen != -1) {
        JOB_LOG_INFO("CartPole Solved at Gen {} | Score: {}", solvedGen, bestFit);
    } else {
        JOB_LOG_WARN("CartPole Best Run: {}", bestFit);
    }

    // Even if not perfect 100, it should be well above random (20)
    REQUIRE(bestFit >= 99.9f);
}
TEST_CASE("Evolution: Edge Cases (Single Thread / Pop 1)", "[coach][edge]")
{
    job::threads::JobStealerCtx ctx(1); // Single thread

    ESCoach::Config cfg;
    cfg.populationSize = 1;
    cfg.sigma = 0.5f;
    cfg.envConfig.type = LearnType::XOR;


    ESCoach coach(ctx.pool, cfg);
    Genome seed = buildXORGenome();

    // Should run without crashing
    Genome survivor = coach.coach(seed);

    REQUIRE(survivor.weights.size() == seed.weights.size());
    REQUIRE(coach.generation() == 1);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Evolution: Flywheel Throughput", "[coach][benchmark]")
{
    job::threads::JobStealerCtx ctx(8);

    // Benchmark XOR (High Frequency, Small Net)
    Genome xorSeed = buildXORGenome();
    BENCHMARK("XOR (Pop=4096)") {
        ESCoach::Config cfg;
        cfg.populationSize = 4096;
        cfg.envConfig.type = LearnType::XOR;
        ESCoach coach(ctx.pool, cfg);
        return coach.coach(xorSeed);
    };

    // Benchmark CartPole (Physics + Inference)
    Genome cartSeed = buildCartPoleGenome();
    BENCHMARK("CartPole (Pop=1024)") {
        ESCoach::Config cfg;
        cfg.populationSize = 1024;
        cfg.envConfig.type  = LearnType::CartPole;
        ESCoach coach(ctx.pool, cfg);
        return coach.coach(cartSeed);
    };
}

#endif
