#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Include benchmark header if compiled with benchmarks enabled
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ctx/job_stealing_ctx.h>

#include "es_coach.h"
#include "portal_evaluator.h"
#include "layer_factory.h"

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;

// Helper to build variable size XOR-like brains
Genome buildXORGenome(int hiddenSize = 8)
{
    Genome g;

    // Layer 1: Input(2) -> Hidden(N)
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = comp::ActivationType::ReLU;
    l1.inputs = 2;
    l1.outputs = hiddenSize;

    l1.weightCount = (2 * hiddenSize) + hiddenSize;
    l1.weightOffset = 0;

    g.architecture.push_back(l1);

    // Layer 2: Hidden(N) -> Output(1)
    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Identity;
    l2.inputs = hiddenSize;
    l2.outputs = 1;

    l2.weightCount = (hiddenSize * 1) + 1;
    l2.weightOffset = l1.weightOffset + l1.weightCount;

    g.architecture.push_back(l2);

    // Resize and Randomize
    size_t total = l1.weightCount + l2.weightCount;
    g.weights.resize(total);
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;

    return g;
}


TEST_CASE("Evolution: Solving XOR", "[coach][xor][usage]")
{
    JOB_LOG_INFO("Starting XOR Functional Test");

    job::threads::JobStealerCtx ctx(8);

    PortalEvaluator physics(ctx.pool);
    physics.setDataset({
        { {0.0f, 0.0f}, {0.0f} },
        { {0.0f, 1.0f}, {1.0f} },
        { {1.0f, 0.0f}, {1.0f} },
        { {1.0f, 1.0f}, {0.0f} }
    });

    Genome seed = buildXORGenome(8); // Standard 2->8->1 network

    ESCoach::Config cfg;
    cfg.populationSize = 512; // Proven stable size
    cfg.sigma = 0.1f;
    cfg.decay = 0.99f;

    ESCoach coach(ctx.pool, cfg);

    float bestFit = 0.0f;
    const int generations = 100;

    for (int i = 0; i < generations; ++i) {
        coach.coach(
            (i == 0) ? seed : coach.bestGenome(),
            physics.portal()
            );

        bestFit = coach.currentBestFitness();

        if (bestFit > 0.95f) {
            JOB_LOG_INFO("XOR Solved at Gen {} Score: {}", i, bestFit);
            break;
        }
    }

    REQUIRE(bestFit > 0.85f);
}


TEST_CASE("Evolution: Edge Cases (Single Survivor)", "[coach][xor][edge]")
{
    // Can we run with a population of 1? (Hill Climbing)
    job::threads::JobStealerCtx ctx(1);
    PortalEvaluator physics(ctx.pool); // Reuse pool is risky but valid for size 1
    physics.setDataset({ {{0.0f, 0.0f}, {0.0f}} }); // Trivial dataset

    Genome seed = buildXORGenome(4);
    ESCoach::Config cfg;
    cfg.populationSize = 1; // <--- The Edge Case
    cfg.sigma = 0.5f;

    ESCoach coach(ctx.pool, cfg);

    // Should not crash
    Genome survivor = coach.coach(seed, physics.portal());

    REQUIRE(survivor.weights.size() == seed.weights.size());
    REQUIRE(coach.generation() == 1);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Evolution: Throughput Benchmarks", "[coach][benchmark]")
{
    job::threads::JobStealerCtx ctx(8); // 8 Cores for Evolution
    PortalEvaluator physics(nullptr);
    physics.setDataset({
        { {0.0f, 0.0f}, {0.0f} },
        { {0.0f, 1.0f}, {1.0f} },
        { {1.0f, 0.0f}, {1.0f} },
        { {1.0f, 1.0f}, {0.0f} }
    });

    Genome seed = buildXORGenome(8);
    BENCHMARK("Evolution Step (Pop=4096, Tiny Net)") {
        ESCoach::Config cfg;
        cfg.populationSize = 4096;
        cfg.sigma = 0.1f;

        ESCoach coach(ctx.pool, cfg);
        return coach.coach(seed, physics.portal());
    };

    // The Big Brain
    Genome fatSeed = buildXORGenome(256);
    BENCHMARK("Evolution Step (Pop=512, Wide Net[256])") {
        ESCoach::Config cfg;
        cfg.populationSize = 512;
        cfg.sigma = 0.1f;

        ESCoach coach(ctx.pool, cfg);
        return coach.coach(fatSeed, physics.portal());
    };
}

#endif
