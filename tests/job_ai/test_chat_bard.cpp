#include <catch2/catch_test_macros.hpp>

#include <job_logger.h>

#include <job_stealing_ctx.h>

#include <es_coach.h>
#include <learn_factory.h>
#include <layer_factory.h>

using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::evo;
using namespace job::ai::layers;
using namespace job::ai::learn;
using namespace job::threads;

// 1. Define the Script (The Training Data)
// We repeat patterns so the Bard learns the rules of the game.
// Rule 1: After <User> comes a question.
// Rule 2: After <Bard> comes an answer.
// Rule 3: Stop after the answer.

const std::string kChatCorpus =
    "<User>Who are you?\n"
    "<Bard>I am the Physics Bard.\n";
// "<User>What are you made of?\n"
// "<Bard>I am made of code and math.\n"
// "<User>Who built you?\n"
// "<Bard>I was built by Joseph.\n"
// "<User>Do you like C++?\n"
// "<Bard>I love C++ templates.\n"
// "<User>What is 1+1?\n"
// "<Bard>It is 2.\n"
// // Repeat a bit to reinforce the pattern if the dataset is tiny
// "<User>Who are you?\n"
// "<Bard>I am the Physics Bard.\n";

Genome buildChatBardGenome(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 256;

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

TEST_CASE("Chat Bard: Learning to Converse", "[ai][chat]")
{
    job::threads::JobStealerCtx ctx(8);

    // 2. Setup Config
    // We use Motif tokens so it learns "<User>" and "<Bard>" as single blocks.
    auto learnCfg = LearnPresets::BardConfig(kChatCorpus.c_str(), job::ai::token::TokenType::Motif);
    learnCfg.contextLen =  32; // Larger context to see the whole Q&A pair
    learnCfg.corpusLen = kChatCorpus.size() - learnCfg.contextLen;
    learnCfg.maxSteps   = 1000;
    learnCfg.seed       = 42;
    learnCfg.initWsMb   = 4;

    ESCoach::Config coachCfg;
    coachCfg.populationSize = 64;
    coachCfg.sigma          = 0.1f;
    coachCfg.decay          = 0.99f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);
    auto *bard = dynamic_cast<BardLearn*>(coach.learner());

    Genome genome = buildChatBardGenome(bard->inputDimension(), bard->outputDimension());


    JOB_LOG_INFO("--- Network Architecture ---");
    JOB_LOG_INFO("Input Dim:  {}", bard->inputDimension());
    JOB_LOG_INFO("Hidden Dim: {}", 64);
    JOB_LOG_INFO("Output Dim: {}", bard->outputDimension());
    JOB_LOG_INFO("Total Weights: {}", genome.weights.size());

    if (genome.weights.size() > 100000)
        JOB_LOG_WARN("Genome is massive! Training will take a while. Weights Size: {}", genome.weights.size());


    auto *tok = bard->tokenizer();

    auto *motifTok = dynamic_cast<job::ai::token::MotifToken*>(tok);

    // Pre-Train
    if (motifTok) {
        motifTok->setCorpus(kChatCorpus, ctx.pool);

        JOB_LOG_INFO("--- The Chemist is distilling the corpus ---");

        // Scratchpad for the encoder to write to (we discard this data, we just want the side-effects)
        std::vector<job::ai::token::ByteLattice> scratchPad(1024);

        // Cast the raw string to bytes for the encoder
        std::span<const uint8_t> corpusBytes(
            reinterpret_cast<const uint8_t*>(kChatCorpus.data()),
            kChatCorpus.size()
            );

        for (int i = 0; i < 500; ++i) {
            // 1. Find a new word
            motifTok->mutate(i * 1234567ULL);
            (void)motifTok->encode(corpusBytes, scratchPad);
            if (i % 50 == 0)
                motifTok->decay();
        }

        motifTok->debug();
    }


    // Train
    uint32_t gen = 0;
    while (!bard->done() && gen < learnCfg.maxSteps) {
        genome = coach.coach((gen==0) ? genome : coach.bestGenome());

        // Optional: Watch it learn in real-time
        if (gen % 10 == 0) {
            std::string sample = bard->say(learnCfg.corpusLen, genome);
            JOB_LOG_INFO("[Gen {}] Sample: {}", gen, sample);
        }
        gen++;
    }

    JOB_LOG_INFO("--- Conversation Test ---");

    // 1. Construct the prompt manually
    // We convert the string prompt into the Token IDs we just learned
    std::vector<float> contextInput;

    // Encode "<User>Who are you?\n<Bard>"
    // This forces the AI to be in the "Answer" state
    std::string promptStr = "<User>Who are you?\n<Bard>";
    std::vector<job::ai::token::ByteLattice> lattice(128);
    tok->encode(
        std::span((const uint8_t*)promptStr.data(), promptStr.size()),
        lattice
        );

    // Flatten Lattice to floats for the network input
    // (Assuming your BardLearn::say / predict methods handle this,
    //  but for a raw test we might need to push this into the context buffer)

    // If BardLearn has a setContext() or similar:
    // bard->setContext(promptStr);

    // The Turing Test (Inference)
    // Then generate
    std::string finalThought = bard->say(learnCfg.corpusLen, genome);
    JOB_LOG_INFO("Generated:\n{}", finalThought);


    // // Check if it learned the tags
    // REQUIRE(finalThought.find("<User>") != std::string::npos);
    // REQUIRE(finalThought.find("<Bard>") != std::string::npos);
}
