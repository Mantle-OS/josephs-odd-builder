#include "job_parallel_for.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <cmath>
#include <numeric>

#include <ctx/job_stealing_ctx.h>
#include <es_coach.h>
#include <genome.h>

using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::threads;


// HELPER: Dummy Problem "Target Sum" Fitness = 1.0 / (1.0 + error)
float evaluate_sum(const Genome& g) {
    float sum = 0.0f;
    for(float w : g.weights)
        sum += w;
    float diff = std::abs(sum - 100.0f); // Target is 100
    return 1.0f / (1.0f + diff);
}

TEST_CASE("ESCoach: Optimizes simple Target Sum problem", "[ai][coach][es][usage]") {
    JobStealerCtx ctx(4);

    //10 weights, initialized to 0
    Genome parent;
    parent.header.uuid = 1;
    parent.weights.resize(10, 0.0f);

    // Put me in the game coach !
    auto cfg = CoachPresets::kStandard;
    cfg.populationSize = 32;
    cfg.sigma = 2.0f; // Start with aggressive mutation

    ESCoach coach(ctx.pool, cfg);

    float initialFitness = evaluate_sum(parent);
    REQUIRE(initialFitness < 0.01f);

    // 30 generations
    for(int i = 0; i < 50; ++i)
        parent = coach.step(parent, evaluate_sum);

    INFO("Generation: " << coach.generation());
    INFO("Final Fitness: " << coach.currentBestFitness());

    REQUIRE(coach.currentBestFitness() > 0.1f);
    REQUIRE(coach.currentBestFitness() > initialFitness);

    float sum = 0.0f;
    for(float w : parent.weights) sum += w;
    REQUIRE(std::abs(sum - 100.0f) < 10.0f);
}

TEST_CASE("ESCoach: Minimization Mode works", "[ai][coach][es][usage]")
{
    JobStealerCtx ctx(4);

    // Problem: Minimize sum (Target 0). Start at 100.
    Genome parent;
    parent.weights.resize(10, 10.0f); // Sum = 100

    auto eval_minimize = [](const Genome &g) {
        float sum = 0.0f;
        for(float w : g.weights)
            sum += std::abs(w);

        return sum;
    };

    auto cfg = CoachPresets::kStandard;
    cfg.populationSize = 32;
    cfg.sigma = 1.0f;

    ESCoach coach(ctx.pool, cfg);
    coach.setMode(OptimizationMode::Minimize);

    float startScore = eval_minimize(parent); // 100
    REQUIRE(startScore == Catch::Approx(100.0f));

    for(int i=0; i<20; ++i)
        parent = coach.step(parent, eval_minimize);


    REQUIRE(coach.currentBestFitness() < startScore);
}


TEST_CASE("ESCoach: Handles Population Size 1(like me lol)", "[ai][coach][es][edge]")
{
    JobStealerCtx ctx(2);

    Genome parent;
    parent.weights.resize(5, 0.0f);

    auto cfg = CoachPresets::kStandard;
    cfg.populationSize = 1;

    ESCoach coach(ctx.pool, cfg);

    parent = coach.step(parent, evaluate_sum);
    REQUIRE(coach.generation() == 1);
}

TEST_CASE("ESCoach: Handles Zero Sigma (Stagnation)", "[ai][coach][es][edge]")
{
    JobStealerCtx ctx(2);

    Genome parent;
    parent.weights.resize(5, 1.0f);

    auto cfg = CoachPresets::kStandard;
    cfg.sigma = 0.0f; // No mutation

    ESCoach coach(ctx.pool, cfg);

    float startScore = evaluate_sum(parent);
    parent = coach.step(parent, evaluate_sum);

    REQUIRE(coach.currentBestFitness() == Catch::Approx(startScore));
}

// TEST_CASE("ESCoach: High-Throughput Population Benchmark", "[ai][coach][es][bench]")
// {
//     JobStealerCtx ctx(8); //8 cores

//     Genome parent;
//     parent.weights.resize(1024); // 1KB Genome
//     std::fill(parent.weights.begin(), parent.weights.end(), 0.5f);

//     auto cfg = CoachPresets::kStandard;
//     cfg.populationSize = 1024; // 1024 mutants per gen

//     ESCoach coach(ctx.pool, cfg);

//     // "fast"
//     auto fastEval = [](const Genome &g) {
//         return g.weights[0] + g.weights[1];
//     };

//     BENCHMARK("ES Generation (Pop=1024, 1KB Genome)") {
//         return coach.step(parent, fastEval);
//     };
// }

TEST_CASE("ESCoach: infra tax vs raw eval", "[ai][coach][es][bench-tax]")
{
    JobStealerCtx ctx(8);
    const int pop = 1024;

    std::vector<Genome> population(pop);
    for (auto &g : population) {
        g.weights.resize(1024, 0.5f);
    }

    auto fastEval = [](const Genome &g) {
        return g.weights[0] + g.weights[1];
    };

    std::vector<float> scores(pop);

    BENCHMARK("Raw parallel eval (Pop=1024, 1KB)") {
        parallel_for(*ctx.pool,
                     size_t{0}, size_t(pop),
                     [&](size_t i) {
                         scores[i] = fastEval(population[i]);
                     });
        return scores[0];
    };
}


TEST_CASE("ESCoach: High-Throughput Population Benchmark", "[ai][coach][es][bench]")
{
    JobStealerCtx ctx(8); //8 cores

    Genome parent;
    parent.weights.resize(1024); // 1KB Genome
    std::fill(parent.weights.begin(), parent.weights.end(), 0.5f);

    auto cfg = CoachPresets::kStandard;
    cfg.populationSize = 1024; // 1024 mutants per gen

    ESCoach coach(ctx.pool, cfg);

    auto fastEval = [](const Genome &g) {
        return g.weights[0] + g.weights[1];
    };

    BENCHMARK("ES Generation (Pop=1024, 1KB Genome)") {
        return coach.step(parent, fastEval);
    };
}


