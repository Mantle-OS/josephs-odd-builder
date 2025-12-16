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

// Helper to build a compatible brain for the environment
Genome buildAsciiBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 32;

    // Layer 1: Perception (Physics -> Features)
    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish; // Good for physics
    l1.inputs       = inputSize;
    l1.outputs      = kHidden;
    l1.weightCount  = (l1.inputs * l1.outputs) + l1.outputs;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    // Layer 2: Projection (Features -> Motif Plane)
    LayerGene l2{};
    l2.type         = LayerType::Dense;
    l2.activation   = comp::ActivationType::Identity; // Logits for Softmax
    l2.inputs       = kHidden;
    l2.outputs      = outputSize;
    l2.weightCount  = (l2.inputs * l2.outputs) + l2.outputs;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    // Xavier/Glorot-ish initialization for stability
    float scale = 1.0f / std::sqrt((float)kHidden);
    for(auto &w : g.weights)
        w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;

    return g;
}

TEST_CASE("Bard Ascii: Trains the Ascii Bard", "[ai][coach][bard]")
{

    JobStealerCtx ctx(8);
    std::string finalWords = "JOSEPHJosephs Odd Builder!"; // "The portal is open. Time flows forward. The dragons are slain.";

    // Config: Use CHAR tokens (Output Dim ~128)
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Ascii
        );
    learnCfg.contextLen = 6; // should this be finalWords.size() ?
    learnCfg.seed       = 42;
    learnCfg.initWsMb   = 1;
    learnCfg.type       = LearnType::Bard;

    // Coach: Bigger population for better search
    ESCoach::Config coachCfg;
    coachCfg.populationSize = 32;   // More agents to find the specific chars
    coachCfg.sigma          = 0.05f; // Finer mutations
    coachCfg.decay          = 0.995f;// Slower cooling
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto* bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim  = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();

    Genome bestGenome = buildAsciiBardGenome(inDim, outDim);
    float bestFitness = 0.0f;

    const int cadence = std::max<int>(finalWords.size(), 1);

    JOB_LOG_INFO("[Drunk Bard Integration] Starts drinking...");

    //1000
    for(int gen = 0; gen < 2000; ++gen) {
        Genome survivor = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());

        bestFitness = coach.currentBestFitness();
        bestGenome  = survivor;

        if (gen % cadence == 0) {
            (void)bard->learn(survivor);
            std::string thought = bard->hallucinate(60);
            // std::replace(thought.begin(), thought.end(), '\n', ' ');
            // JOB_LOG_INFO("Gen {} | Fit: {} | Say: \"{}\"", gen, bestFitness, thought);
            if (bestFitness > 96.0f) // I could drop this down to say 97% depends on how "close" to real it "sounds"
                break;
        }
    }

    (void)bard->learn(coach.bestGenome());
    std::string finalThought = bard->hallucinate((int)(finalWords.size() -6));

    JOB_LOG_INFO("[Drunk Bard Integration] Final: {}", finalThought);
    REQUIRE(bestFitness > 80);
}
