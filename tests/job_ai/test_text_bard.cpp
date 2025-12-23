#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <job_stealing_ctx.h>
#include <iostream>
#include <algorithm>

#include <job_logger.h>

#include <job_fifo_ctx.h>
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

inline std::string finalWords = "JOSEPHJoseph Odd Builder !";


Genome buildAsciiBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 64;

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

Genome buildTextBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 32;

    // Layer 1: Perception
    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish;
    l1.inputs       = inputSize;
    l1.outputs      = kHidden;
    l1.weightCount  = (l1.inputs * l1.outputs) + l1.outputs;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    // Layer 2: Prediction
    LayerGene l2{};
    l2.type         = LayerType::Dense;
    l2.activation   = comp::ActivationType::Identity;
    l2.inputs       = kHidden;
    l2.outputs      = outputSize;
    l2.weightCount  = (l2.inputs * l2.outputs) + l2.outputs;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    g.weights.resize(l1.weightCount + l2.weightCount);

    // Xavier Init
    float scale = 1.0f / std::sqrt((float)kHidden);
    for(auto &w : g.weights)
        w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;

    return g;
}

TEST_CASE("Bard Byte: Trains the Byte Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Byte
        );
    learnCfg.contextLen     = 6;                                                // Context Window ? IE 1st 6 chars of the finalWords
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);      // the length of the corpus - the context window size (maybe should be doubled ?)
    learnCfg.seed           = 42;                                               // The seed
    learnCfg.initWsMb       = 1;                                                // This is the workspace memory X * 1024 *1024
    learnCfg.maxSteps       = 2000;                                             // How many loops
    learnCfg.targetFitness  = 98.0f;                                            // What we are aiming for
    learnCfg.type           = LearnType::Bard;                                  // The type of learner

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 16;
    coachCfg.sigma          = 0.09f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();
    REQUIRE(outDim == 256);

    Genome bestGenome = buildTextBardGenome(inDim, outDim);
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Byte Bard] Starts drinking...");

    while(!bard->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        thought = bard->say(learnCfg.corpusLen, bestGenome);
        gen++;
    }
    JOB_LOG_INFO("[Byte Bard] Final: {}", thought.substr(0, 20));
    REQUIRE(bard->fitness() > 80);
}

TEST_CASE("Bard Ascii: Trains the Ascii Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Ascii
        );
    learnCfg.contextLen     = 6;
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);
    learnCfg.seed           = 42;
    learnCfg.initWsMb       = 1;
    learnCfg.maxSteps       = 2000;
    learnCfg.targetFitness  = 98.0f;
    learnCfg.type           = LearnType::Bard;

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 64;
    coachCfg.sigma          = 0.09f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();
    REQUIRE(outDim == 95);

    Genome bestGenome = buildAsciiBardGenome(inDim, outDim);
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Ascii Bard] Starts drinking...");

    while(!bard->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        thought = bard->say(learnCfg.corpusLen, bestGenome);
        gen++;
    }
    JOB_LOG_INFO("[Ascii Bard] Final: {}", thought.substr(0, 20));
    REQUIRE(bard->fitness() > 80);
}




TEST_CASE("Char Byte: Trains the Char Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Char
        );
    learnCfg.contextLen     = 6;
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);
    learnCfg.seed           = 42;
    learnCfg.initWsMb       = 1;
    learnCfg.maxSteps       = 2000;
    learnCfg.targetFitness  = 98.0f;
    learnCfg.type           = LearnType::Bard;

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 32;
    coachCfg.sigma          = 0.09f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *bard = dynamic_cast<BardLearn*>(coach.learner());
    REQUIRE(bard != nullptr);

    const uint32_t inDim  = bard->inputDimension();
    const uint32_t outDim = bard->outputDimension();
    REQUIRE(outDim == 256);

    Genome bestGenome = buildTextBardGenome(inDim, outDim);
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Char Bard] Starts drinking...");

    while(!bard->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        thought = bard->say(learnCfg.corpusLen, bestGenome);
        gen++;
    }
    JOB_LOG_INFO("[Char Bard] Final: {}", thought.substr(0, 20));
    REQUIRE(bard->fitness() > 80);
}

TEST_CASE("Motif Tokens: Trains the Physics Bard", "[ai][coach][bard]")
{
    JobStealerCtx ctx(8);
    std::string finalWords = "JOSEPHJoseph Odd Builder !";                      // The context window (JOSEPH)6 then the corpus 20
    auto learnCfg = LearnPresets::BardConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Motif
        );
    learnCfg.contextLen     = 6;                                                // Context Window ? IE 1st 6 chars of the finalWords
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);      // the length of the corpus - the context window size (maybe should be doubled ?)
    learnCfg.seed           = 42;                                               // The seed
    learnCfg.initWsMb       = 1;                                                // This is the workspace memory X * 1024 *1024
    learnCfg.maxSteps       = 200;                                              // How many loops
    learnCfg.targetFitness  = 98.0f;                                            // What we are aiming for
    learnCfg.type           = LearnType::Bard;                                  // The typ0e of learner

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 8;                                                // How big is this
    coachCfg.sigma          = 0.1f;                                             // sigma push
    coachCfg.decay          = 0.99f;                                            // decay rate
    coachCfg.envConfig      = learnCfg;                                         // the Learn Config

    ESCoach coach(ctx.pool, coachCfg);                                          // Create the Coach

    auto *bard = dynamic_cast<BardLearn*>(coach.learner());                     // get the bard from the coach
    REQUIRE(bard != nullptr);

    const uint32_t inDim = bard->inputDimension();                              // Dim In
    const uint32_t outDim = bard->outputDimension();                            // Dim Out

    Genome bestGenome = buildTextBardGenome(inDim, outDim);                     // Create the Genome's with with the layers and also activation type
    std::string thought{};
    uint32_t gen = 0;
    JOB_LOG_INFO("[Physics Bard] Starts drinking...");
    while(!bard->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome()); // Evolve/train one generation save as survivor (which will later be the step fo non volitile)
        thought = bard->say(learnCfg.corpusLen, bestGenome);                    // Get the output of what it just learned (should be 14 ? )
        gen++;
    }

    JOB_LOG_INFO("[Physics Bard] Final: {}", thought.substr(0, 20));
    REQUIRE(bard->fitness() > 80.0f);
}

TEST_CASE("Reincarnation: Saving and Loading the Physics Bard", "[ai][bard][io]")
{
    std::string vocabFile = "physics_vocab.bin";
    std::string brainFile = "physics_brain.bin";

    // Cleanup old files
    std::remove(vocabFile.c_str());
    std::remove(brainFile.c_str());

    // The First Life (Training)
    {
        JobStealerCtx ctx(8);
        auto learnCfg = LearnPresets::BardConfig(finalWords.c_str(), job::ai::token::TokenType::Motif);
        learnCfg.contextLen = 6;
        learnCfg.corpusLen  = (finalWords.length() - learnCfg.contextLen);
        learnCfg.maxSteps = 250; // Ensure convergence
        learnCfg.seed = 42;

        ESCoach::Config coachCfg;
        coachCfg.populationSize = 8;
        coachCfg.sigma          = 0.1f;
        coachCfg.decay          = 0.99f;
        coachCfg.envConfig = learnCfg;

        ESCoach coach(ctx.pool, coachCfg);
        auto *bard = dynamic_cast<BardLearn*>(coach.learner());

        // Train
        const uint32_t inDim = bard->inputDimension();
        const uint32_t outDim = bard->outputDimension();
        Genome genome = buildTextBardGenome(inDim, outDim);

        uint32_t gen = 0;
        while (!bard->done() && gen < learnCfg.maxSteps) {
            genome = coach.coach((gen==0) ? genome : coach.bestGenome());
            (void)bard->say(learnCfg.corpusLen, genome);
            gen++;
        }

        // Verify Life 1 worked
        REQUIRE(bard->fitness() > 80.0f);
        Genome survivorGenome = coach.bestGenome();

        // SAVE THE DICTIONARY
        std::ofstream vFile(vocabFile, std::ios::binary);
        REQUIRE(vFile.is_open());
        REQUIRE(bard->tokenizer()->save(vFile));
        vFile.close(); // Flush

        // SAVE THE BRAIN
        REQUIRE(GenomeSerializer::save(survivorGenome, brainFile));

        JOB_LOG_INFO("[Life 1] Saved Bard to disk.");
    } // Bard and Coach destroyed here

    // The Second Life (Inference Only)
    {
        JOB_LOG_INFO("[Life 2] Waking up from disk...");
        JobFifoCtx ctx(1);
        // Load Brain
        Genome reincarnatedGenome = GenomeSerializer::load(brainFile);
        REQUIRE(reincarnatedGenome.weights.size() > 0);
        REQUIRE(reincarnatedGenome.architecture.size() > 0);

        // Setup Learner (Simulate App Start)
        auto learnCfg = LearnPresets::BardConfig(finalWords.c_str(), job::ai::token::TokenType::Motif);
        learnCfg.contextLen = 6;
        learnCfg.corpusLen = (uint32_t)(finalWords.length() - learnCfg.contextLen);
        // Fresh bard, empty mind
        BardLearn newBard(learnCfg, ctx.pool);

        std::ifstream vFile(vocabFile, std::ios::binary);
        REQUIRE(vFile.is_open());
        REQUIRE(newBard.tokenizer()->load(vFile));
        vFile.close();

        // The new bard runs the old brain on the old dictionary.
        std::string thought = newBard.say(learnCfg.corpusLen, reincarnatedGenome);

        JOB_LOG_INFO("[Life 2] Recalled: {}", thought);

        // Check for success
        REQUIRE(thought.size() > 0);
        REQUIRE(thought.find("Joseph") != std::string::npos);
    }
}
