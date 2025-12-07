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

    l1.weight_count = (2 * 8) + 8; // Weights + Bias
    l1.weight_offset = 0;          // Starts at 0 [CORRECT]

    g.architecture.push_back(l1);

    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Identity;
    l2.inputs = 8;
    l2.outputs = 1;

    l2.weight_count = (8 * 1) + 1;
    l2.weight_offset = l1.weight_offset + l1.weight_count;

    g.architecture.push_back(l2);

    // Resize and Randomize
    size_t total = l1.weight_count + l2.weight_count;
    g.weights.resize(total);
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f) * 0.5f;

    return g;
}

TEST_CASE("Evolution: Solving XOR", "[coach][xor]")
{
    job::threads::JobStealerCtx ctx(1);

    PortalEvaluator physics(ctx.pool);
    physics.setDataset({
        { {0.0f, 0.0f}, {0.0f} },
        { {0.0f, 1.0f}, {1.0f} },
        { {1.0f, 0.0f}, {1.0f} },
        { {1.0f, 1.0f}, {0.0f} }
    });

    Genome seed = buildXORGenome();

    ESCoach::Config cfg;
    cfg.populationSize = 256;
    cfg.sigma = 0.1f;
    cfg.decay = 0.99f;

    ESCoach coach(ctx.pool, cfg);

    float bestFit = 0.0f;
    const int generations = 100;

    for (int i = 0; i < generations; ++i) {
        // winner winner chicken dinner
        Genome winner = coach.coach(
            (i == 0) ? seed : coach.bestGenome(),
            physics.portal()
            );

        bestFit = coach.currentBestFitness();

        // pray !@!
        if (bestFit > 0.95f) {
            JOB_LOG_INFO("XOR Solved at Gen {} Score: ", i, bestFit);
            break;
        }
    }

    REQUIRE(bestFit > 0.85f); // hopefully close to 1.0
}
