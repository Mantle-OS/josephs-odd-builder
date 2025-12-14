#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <job_stealing_ctx.h>
#include <iostream>
#include <algorithm>
#include <job_logger.h>

// The Core Stack
#include <es_coach.h>
#include <learn_type.h>
#include <layer_factory.h>
#include <learn_bard.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;
using namespace job::ai::learn;
using namespace job::threads;

Genome buildBardGenome()
{
    Genome g;
    // Context (12) -> Hidden (128) -> Vocab (95)
    constexpr int kIn       = BardLearn::kContextLen;
    constexpr int kOut      = BardLearn::kVocabSize;
    constexpr int kHidden   = 128;

    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish;
    l1.inputs       = kIn;
    l1.outputs      = kHidden;
    l1.weightCount  = (kIn * kHidden) + kHidden;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    LayerGene l2{};
    l2.type         = LayerType::Dense;
    l2.activation   = comp::ActivationType::Identity;
    l2.inputs       = kHidden;
    l2.outputs      = kOut;
    l2.weightCount  = (kHidden * kOut) + kOut;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    for(auto &w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f);

    return g;
}

TEST_CASE("ESCoach: Trains the Drunk Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);

    ESCoach::Config cfg;
    cfg.populationSize = 64;      // Good balance for speed/search ?
    cfg.sigma          = 0.05f;    // Aggressive start ?
    cfg.decay          = 0.995f;   // Slow cool down ?
    cfg.taskType       = LearnType::Bard;
    cfg.memLimitMB     = 1;

    ESCoach coach(ctx.pool, cfg);

    auto *baseLearner = coach.learner();
    REQUIRE(baseLearner != nullptr);

    auto *bard = dynamic_cast<BardLearn*>(baseLearner);
    REQUIRE(bard != nullptr);

    Genome bestGenome = buildBardGenome();
    float bestFitness = 0.0f;

    JOB_LOG_INFO("[Drunk Bard Integration] Starts drinking...");

    for(int gen = 0; gen < 1000; ++gen) {
        Genome survivor = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());

        bestFitness = coach.currentBestFitness();
        bestGenome  = survivor;

        if (gen % 50 == 0) {
            (void)bard->learn(survivor, cfg.memLimitMB);

            std::string thought = bard->hallucinate(60);
            std::replace(thought.begin(), thought.end(), '\n', ' ');

            JOB_LOG_INFO("Gen {} | Fit: {} | Say: \"{}\"", gen, bestFitness, thought);
            if (bestFitness > 0.90f)
                break;
        }
    }

    (void)bard->learn(coach.bestGenome());
    std::string finalThought = bard->hallucinate(60);

    JOB_LOG_INFO("[Drunk Bard Integration] Final: {}", finalThought);
    REQUIRE(bestFitness > 0.80f);
}
