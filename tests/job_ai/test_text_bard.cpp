#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <job_stealing_ctx.h>
#include <iostream>
#include <algorithm>
#include <job_logger.h>

// The Core Stack
#include <es_coach.h>
#include <learn_factory.h>
#include <layer_factory.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;
using namespace job::ai::learn;
using namespace job::threads;

Genome buildTextBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    // BUMPED FROM 32 TO 64
    constexpr int kHidden = 32;

    // Layer 1: Perception
    LayerGene l1{};
    l1.type          = LayerType::Dense;
    l1.activation    = comp::ActivationType::Swish;
    l1.inputs        = inputSize;
    l1.outputs       = kHidden;
    l1.weightCount   = (l1.inputs * l1.outputs) + l1.outputs;
    l1.weightOffset  = 0;
    g.architecture.push_back(l1);

    // Layer 2: Prediction
    LayerGene l2{};
    l2.type          = LayerType::Dense;
    l2.activation    = comp::ActivationType::Identity;
    l2.inputs        = kHidden;
    l2.outputs       = outputSize;
    l2.weightCount   = (l2.inputs * l2.outputs) + l2.outputs;
    l2.weightOffset  = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    // Xavier Init
    float scale = 1.0f / std::sqrt((float)kHidden);
    for(auto &w : g.weights) {
        w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;
    }

    return g;
}


TEST_CASE("Motif Tokens: Trains the Physics Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    std::string finalWords = "The portal is open. Time flows forward. The dragons are slain.";
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Motif
        );
    learnCfg.contextLen = 6;
    learnCfg.seed       = 42;
    learnCfg.initWsMb   = 8;
    learnCfg.type       = LearnType::Bard;

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 8;
    coachCfg.sigma          = 0.1f;
    coachCfg.decay          = 0.99f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto* bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim  = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();

    Genome bestGenome = buildTextBardGenome(inDim, outDim);
    float bestFitness = 0.0f;

    JOB_LOG_INFO("[Physics Bard] Input: {} -> Output: {} (Motif Plane)", inDim, outDim);
    JOB_LOG_INFO("[Physics Bard] Corpus: \"{}\"", learnCfg.corpus);
    JOB_LOG_INFO("[Physics Bard] Starts drinking...");

    // Use corpusSize cadence like the original test.
    // Ensure it's never 0.
    const int cadence = std::max<int>(finalWords.size(), 1);

    for (int gen = 0; gen < 200; ++gen) {

        // Evolve one generation
        Genome survivor = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());

        bestFitness = coach.currentBestFitness();
        bestGenome  = survivor;

        // Periodic "sober up" + hallucinate on THIS BardLearn instance
        if (gen % cadence == 0) {
            (void)bard->learn(bestGenome);               // primes/loads m_runner for hallucinate
            std::string thought = bard->hallucinate(20); // keep short for logs

            std::replace(thought.begin(), thought.end(), '\n', ' ');
            JOB_LOG_INFO("Gen {:3} | Fit: {:6.2f}% | \"{}\"", gen, bestFitness, thought);

            if (bestFitness > 98.0f) {
                JOB_LOG_INFO("[Physics Bard] Enlightenment achieved at Gen {}", gen);
                break;
            }
        }
    }

    // Final
    (void)bard->learn(bestGenome);
    std::string finalThought = bard->hallucinate(50);
    std::replace(finalThought.begin(), finalThought.end(), '\n', ' ');

    JOB_LOG_INFO("[Physics Bard] Final Hallucination: \"{}\"", finalThought);

    REQUIRE(bestFitness > 50.0f);
}


TEST_CASE("Char Tokens: Trains the Physics Bard", "[ai][coach][bard]")
{
    // 1. Setup
    JobStealerCtx ctx(8);
    std::string finalWords = "The portal is open. Time flows forward. The dragons are slain.";

    // Config: Use CHAR tokens (Output Dim ~128)
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Char
        );
    learnCfg.contextLen = 6;
    learnCfg.seed       = 42;
    learnCfg.initWsMb   = 1; // 1MB is plenty for Char mode (small output layer)
    learnCfg.type       = LearnType::Bard;

    // Coach: Bigger population for better search
    ESCoach::Config coachCfg;
    coachCfg.populationSize = 128;   // More agents to find the specific chars
    coachCfg.sigma          = 0.08f; // Finer mutations
    coachCfg.decay          = 0.999f;// Slower cooling
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto* bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim  = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();

    JOB_LOG_INFO("[Char Bard] Input: {} -> Output: {} (ASCII)", inDim, outDim);
    if (outDim > 200)
        JOB_LOG_WARN("[Char Bard] Output dim seems high ({}) for Char mode?", outDim);

    auto buildStrongGenome = [&](uint32_t i, uint32_t o) -> Genome {
        Genome g;
        constexpr int kHidden = 128; // Increased capacity

        LayerGene l1{};
        l1.type = LayerType::Dense; l1.activation = comp::ActivationType::Swish;
        l1.inputs = i; l1.outputs = kHidden;
        l1.weightCount = (l1.inputs * l1.outputs) + l1.outputs; l1.weightOffset = 0;
        g.architecture.push_back(l1);

        LayerGene l2{};
        l2.type = LayerType::Dense; l2.activation = comp::ActivationType::Identity;
        l2.inputs = kHidden; l2.outputs = o;
        l2.weightCount = (l2.inputs * l2.outputs) + l2.outputs;
        l2.weightOffset = l1.weightCount;
        g.architecture.push_back(l2);

        g.weights.resize(l1.weightCount + l2.weightCount);
        float scale = 1.0f / std::sqrt((float)kHidden);
        for(auto &w : g.weights) w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;
        return g;
    };

    Genome bestGenome = buildStrongGenome(inDim, outDim);
    float bestFitness = 0.0f;

    JOB_LOG_INFO("[Char Bard] Corpus: \"{}\"", learnCfg.corpus);
    JOB_LOG_INFO("[Char Bard] Starts reading...");

    // 3. The Loop
    const int cadence = 50; // Log every 50 gens

    for (int gen = 0; gen < 2000; ++gen) {
        Genome survivor = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        bestFitness = coach.currentBestFitness();
        bestGenome  = survivor;

        if (bestFitness >= 98.0f) {
            (void)bard->learn(bestGenome);
            std::string thought = bard->hallucinate((int)finalWords.size());
            std::replace(thought.begin(), thought.end(), '\n', ' ');
            JOB_LOG_INFO("[Char Bard] Enlightenment at Gen {} | Fit: {:.2f}%", gen, bestFitness);
            JOB_LOG_INFO("[Char Bard] Final: \"{}\"", thought);
            break;
        }

        // Monitoring
        if (gen % cadence == 0) {
            (void)bard->learn(bestGenome);
            std::string thought = bard->hallucinate(30);
            std::replace(thought.begin(), thought.end(), '\n', ' ');
            JOB_LOG_INFO("Gen {:3} | Fit: {:6.2f}% | \"{}\"", gen, bestFitness, thought);
        }
    }

    (void)bard->learn(bestGenome);
    std::string finalThought = bard->hallucinate((int)finalWords.size());
    std::replace(finalThought.begin(), finalThought.end(), '\n', ' ');

    JOB_LOG_INFO("[Char Bard] Result: \"{}\"", finalThought);

    REQUIRE(bestFitness > 90.0f);
}


