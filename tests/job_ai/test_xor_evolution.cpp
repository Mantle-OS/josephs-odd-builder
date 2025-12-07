#include <catch2/catch_test_macros.hpp>
#include <ctx/job_stealing_ctx.h>

#include "es_coach.h"
#include "portal_evaluator.h"
#include "layer_factory.h"

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;




// build the xor brain
Genome buildXORGenome()
{
    Genome g;

    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = comp::ActivationType::ReLU;
    l1.inputs = 2;
    l1.outputs = 8;

    l1.weightCount = (2 * 8) + 8; // Weights + Bias
    l1.weightOffset = 0;          // Starts at 0 [CORRECT]

    g.architecture.push_back(l1);

    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Identity;
    l2.inputs = 8;
    l2.outputs = 1;

    l2.weightCount = (8 * 1) + 1;
    l2.weightOffset = l1.weightOffset + l1.weightCount;

    g.architecture.push_back(l2);

    // Resize and Randomize
    size_t total = l1.weightCount + l2.weightCount;
    g.weights.resize(total);
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;

    return g;
}

TEST_CASE("Evolution: Solving XOR", "[coach][xor]")
{
    JOB_LOG_INFO("Starting XOR This may take some time");
    job::threads::JobStealerCtx ctx(8);
    job::threads::JobStealerCtx evalCtx(1);

    PortalEvaluator physics(evalCtx.pool);
    physics.setDataset({
        { {0.0f, 0.0f}, {0.0f} },
        { {0.0f, 1.0f}, {1.0f} },
        { {1.0f, 0.0f}, {1.0f} },
        { {1.0f, 1.0f}, {0.0f} }
    });

    Genome seed = buildXORGenome();

    ESCoach::Config cfg;
    cfg.populationSize = 512;// 256;
    cfg.sigma = 0.1f;
    cfg.decay = 0.99f;

    ESCoach coach(ctx.pool, cfg);

    float bestFit = 0.0f;
    const int generations = 100;

    for (int i = 0; i < generations; ++i) {
        // winner winner chicken dinner
        JOB_LOG_INFO("XOR Check {}", i);
        Genome winner = coach.coach(
            (i == 0) ? seed : coach.bestGenome(),
            physics.portal()
            );

        bestFit = coach.currentBestFitness();

        // pray !@!
        if (bestFit > 0.95f) {
            JOB_LOG_INFO("XOR Solved at Gen {} Score: {}", i, bestFit);
            break;
        }
    }

    REQUIRE(bestFit > 0.85f); // hopefully close to 1.0
}


