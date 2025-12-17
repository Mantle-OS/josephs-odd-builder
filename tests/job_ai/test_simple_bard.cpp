#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>

// Core
#include <job_logger.h>

// Threads
#include <job_stealing_ctx.h>

// Ai
#include <es_coach.h>
#include <learn_factory.h>
#include <layer_factory.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;
using namespace job::ai::learn;
using namespace job::threads;

Genome buildAsciiBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 32;

    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish;
    l1.inputs       = inputSize;
    l1.outputs      = kHidden;
    l1.weightCount  = (l1.inputs * l1.outputs) + l1.outputs;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    LayerGene l2{};
    l2.type         = LayerType::Dense;
    l2.activation   = comp::ActivationType::Identity;
    l2.inputs       = kHidden;
    l2.outputs      = outputSize;
    l2.weightCount  = (l2.inputs * l2.outputs) + l2.outputs;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    float scale = 1.0f / std::sqrt((float)kHidden);
    for(auto &w : g.weights)
        w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;

    return g;
}

TEST_CASE("Bard Ascii: Trains the Ascii Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    // first 6 are the "context window"
    std::string finalWords = "JOSEPHJosephs Odd Builder!";

    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Ascii
        );
    learnCfg.contextLen = 6;
    learnCfg.corpusLen  = finalWords.size();
    learnCfg.seed       = 42;
    learnCfg.initWsMb   = 1;
    learnCfg.type       = LearnType::Bard;


    ESCoach::Config coachCfg;
    coachCfg.populationSize = 32;
    coachCfg.sigma          = 0.05f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto* bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim  = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();
    REQUIRE(outDim == 95);


    Genome bestGenome = buildAsciiBardGenome(inDim, outDim);
    float bestFitness = 0.0f;

    // const int cadence = std::max<int>(finalWords.size(), 1);

    JOB_LOG_INFO("[Drunk Bard Integration] Starts drinking...");

    for(int gen = 0; gen < 2000; ++gen) {
        Genome survivor = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());

        bestFitness = coach.currentBestFitness();
        bestGenome  = survivor;

        if (gen %  learnCfg.corpusLen  == 0) {
            (void)bard->learn(survivor);
            std::string thought = bard->hallucinate(learnCfg.corpusLen);
            if (bestFitness > 96.0f)
                break;
        }
    }

    (void)bard->learn(coach.bestGenome());
    std::string finalThought = bard->hallucinate((int)(learnCfg.corpusLen - learnCfg.contextLen));

    JOB_LOG_INFO("[Drunk Bard Integration] Final: {}", finalThought);
    REQUIRE(bestFitness > 80);
}
