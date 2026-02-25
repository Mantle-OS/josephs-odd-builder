#include <catch2/benchmark/catch_benchmark.hpp>
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


Genome buildAsciiLayers(uint32_t inputSize, uint32_t outputSize)
{
    Genome g;
    constexpr int kHidden = 64;

    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish;
    l1.inputs       = inputSize;
    l1.outputs      = kHidden;

    g.weights.resize((l1.inputs * l1.outputs) + l1.outputs);

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

Genome buildTextLayers(uint32_t inputSize, uint32_t outputSize)
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

Genome buildPhysicsLanguageGenome(uint32_t inputSize, uint32_t /*outputSize*/, adapters::AdapterType physicsType = adapters::AdapterType::BarnesHut)
{
    Genome g;
    constexpr int kHidden = 32;
    constexpr int kHeads  = 4;

     // Layer 1: Perception
    LayerGene l1{};
    l1.type         = LayerType::Dense;
    l1.activation   = comp::ActivationType::Swish; // 82 3 seconds
    l1.inputs       = inputSize;
    l1.outputs      = kHidden;
    l1.weightCount  = (l1.inputs * l1.outputs) + l1.outputs;
    l1.weightOffset = 0;

    g.weights.resize((l1.inputs * l1.outputs) + l1.outputs);
    g.architecture.push_back(l1);


     // Layer 2: Prediction
    LayerGene lcfg{};
    lcfg.type          = LayerType::Attention;
    lcfg.auxiliaryData = static_cast<uint32_t>(physicsType);
    // lcfg.activation    = comp::ActivationType::Swish;
    lcfg.inputs        = kHidden;
    lcfg.outputs       = kHeads ;
    lcfg.weightCount   = (lcfg.inputs * lcfg.outputs) + lcfg.outputs;
    lcfg.weightOffset  = l1.weightCount;

    g.weights.resize(static_cast<uint32_t>(lcfg.weightCount));

    // Params: 4(kHeads) matrices (Q,K,V,O) + biases
    // Size: 4(kHeads) * (Dim * Dim) + 4(kHeads) * Dim
    size_t attnParams = kHeads * (size_t(kHidden) * kHidden) + kHeads * kHidden;
    lcfg.weightCount   = static_cast<uint32_t>(attnParams);

    g.weights.resize(l1.weightCount + static_cast<uint32_t>(attnParams));
    g.architecture.push_back(lcfg);

    // Xavier Init
    float scale = 1.0f / std::sqrt((float)kHidden);
    for(auto &w : g.weights)
        w = ((rand() % 2000) / 1000.0f - 1.0f) * scale;

    return g;
}


TEST_CASE("Language Byte P=16, N=1000 : Trains using Byte Language", "[ai][coach][llm]")
{
    JobStealerCtx ctx(8);

    // Create the text_encoder
    auto learnCfg = LearnPresets::LanguageConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Byte
        );
    learnCfg.contextLen     = 6;                                                // Context Window ? IE 1st 6 chars of the finalWords
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);      // the length of the corpus - the context window size (maybe should be doubled ?)
    learnCfg.seed           = 42;                                               // The seed
    learnCfg.initWsMb       = 4;                                                // This is the workspace memory X * 1024 *1024
    learnCfg.maxSteps       = 1000;                                             // How many loops
    learnCfg.targetFitness  = 98.0f;                                            // What we are aiming for
    learnCfg.type           = LearnType::Language;                              // The type of learner


    // Create the Coach/Trainer
    ESConfig coachCfg;
    coachCfg.populationSize = 16;
    coachCfg.sigma          = 0.08f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;
    ESCoach coach(ctx.pool, coachCfg);

    // Get the "Learning Type" from the coach.
    auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());
    REQUIRE(llm != nullptr);

    const uint32_t inDim = llm->inputDimension();
    const uint32_t outDim = llm->outputDimension();
    REQUIRE(outDim == 256);

    // Create the Layers
    Genome bestGenome = buildTextLayers(inDim, outDim);

    // Whatever logging for now
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Byte Language] Start training");
    BENCHMARK("Training Loop Po=16: N=1000)") {
        while(!llm->done() && gen < learnCfg.maxSteps ) {
            bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
            gen++;
        }
    };
    thought = llm->say(learnCfg.corpusLen, bestGenome);
    JOB_LOG_INFO("\n[Language Model, Byte Encodings] {} Final: {}", llm->fitness(), thought.substr(0, 20));
    REQUIRE(llm->fitness() > 80);
}


TEST_CASE("Language(With Attention) P=512 N=1000 Model Byte Encodings. ", "[ai][coach][llm]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::LanguageConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Byte
        );
    learnCfg.contextLen     = 6;                                                // Context Window ? IE 1st 6 chars of the finalWords
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);      // the length of the corpus - the context window size (maybe should be doubled ?)
    learnCfg.seed           = 42;                                               // The seed
    learnCfg.initWsMb       = 8;                                                // This is the workspace memory X * 1024 *1024
    learnCfg.maxSteps       = 1000;                                             // How many loops
    learnCfg.targetFitness  = 98.0f;                                            // What we are aiming for
    learnCfg.type           = LearnType::Language;                              // The type of learner


    ESConfig coachCfg;
    coachCfg.populationSize = 512;
    coachCfg.sigma          = 0.08f;
    coachCfg.decay          = 0.995f;

    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());
    REQUIRE(llm != nullptr);

    const uint32_t inDim = llm->inputDimension();
    const uint32_t outDim = llm->outputDimension();
    // REQUIRE(outDim == 256);
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::Dense);         // Sigma: 0.08f || POP: 1028 || Steps 500  || 90.010956,
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::Flash);         // Sigma: 0.08f || POP: 1028 || Steps 550  || 92.649574
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::LowRank);       // Sigma: 0.08f || POP: 1028 || Steps 3000 || 53.09803
    Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::FMM);           // Sigma: 0.08f || POP: 1028 || Steps 350  || 93.06909
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::BarnesHut);     // Sigma: 0.08f || POP: 64   || Steps 500  || 85.5214 // This can not handle large populations
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::Verlet);        // Sigma: 0.08f || POP: 1028 || Steps 1000 || 88.14781
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::RK4);           // Sigma: 0.08f || POP: 1028 || Steps 1000 || 88.475716
    // Genome bestGenome = buildPhysicsLanguageGenome(inDim, outDim, adapters::AdapterType::Stencil);       // Sigma: 0.08f || POP: 1028 || Steps 1000 || 87.72158
    std::string thought{};
    uint32_t gen = 0;

    BENCHMARK("[Byte Attention Language] Starts ...") {
        while(!llm->done() && gen < learnCfg.maxSteps ) {
            bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
            gen++;
        }
    };
    thought = llm->say(learnCfg.corpusLen, bestGenome);

    JOB_LOG_INFO("\n[Language Model, Byte Encodings] {} Final: {}",llm->fitness(), thought.substr(0, 20));
    REQUIRE(llm->fitness() > 80);
}



TEST_CASE("Language Ascii: Trains the Ascii Language", "[ai][coach][llm]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::LanguageConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Ascii
        );
    learnCfg.contextLen     = 6;
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);
    learnCfg.seed           = 42;
    learnCfg.initWsMb       = 1;
    learnCfg.maxSteps       = 2000;
    learnCfg.targetFitness  = 98.0f;
    learnCfg.type           = LearnType::Language;

    ESConfig coachCfg;
    coachCfg.populationSize = 64;
    coachCfg.sigma          = 0.09f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());
    REQUIRE(llm != nullptr);

    const uint32_t inDim = llm->inputDimension();
    const uint32_t outDim = llm->outputDimension();
    REQUIRE(outDim == 95);

    Genome bestGenome = buildAsciiLayers(inDim, outDim);
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Ascii Language] Starts drinking...");

    while(!llm->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        thought = llm->say(learnCfg.corpusLen, bestGenome);
        gen++;
    }
    JOB_LOG_INFO("[Ascii Language] Final: {}", thought.substr(0, 20));
    REQUIRE(llm->fitness() > 80);
}

TEST_CASE("Char Byte: Trains the Char Language", "[ai][coach][llm]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::LanguageConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Char
        );
    learnCfg.contextLen     = 6;
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);
    learnCfg.seed           = 42;
    learnCfg.initWsMb       = 1;
    learnCfg.maxSteps       = 2000;
    learnCfg.targetFitness  = 98.0f;
    learnCfg.type           = LearnType::Language;

    ESConfig coachCfg;
    coachCfg.populationSize = 32;
    coachCfg.sigma          = 0.09f;
    coachCfg.decay          = 0.995f;
    coachCfg.envConfig      = learnCfg;

    ESCoach coach(ctx.pool, coachCfg);

    auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());
    REQUIRE(llm != nullptr);

    const uint32_t inDim  = llm->inputDimension();
    const uint32_t outDim = llm->outputDimension();
    REQUIRE(outDim == 256);

    Genome bestGenome = buildTextLayers(inDim, outDim);
    std::string thought{};
    uint32_t gen = 0;

    JOB_LOG_INFO("[Char Language] Starts drinking...");

    while(!llm->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome());
        thought = llm->say(learnCfg.corpusLen, bestGenome);
        gen++;
    }
    JOB_LOG_INFO("[Char Language] Final: {}", thought.substr(0, 20));
    REQUIRE(llm->fitness() > 80);
}




TEST_CASE("Motif Tokens: Trains the Language Model", "[ai][coach][llm]")
{
    JobStealerCtx ctx(8);
    auto learnCfg = LearnPresets::LanguageConfig(
        finalWords.c_str(),
        job::ai::token::TokenType::Motif
        );
    learnCfg.contextLen     = 6;                                                // Context Window ? IE 1st 6 chars of the finalWords
    learnCfg.corpusLen      = (finalWords.length() - learnCfg.contextLen);      // the length of the corpus - the context window size (maybe should be doubled ?)
    learnCfg.seed           = 42;                                               // The seed
    learnCfg.initWsMb       = 1;                                                // This is the workspace memory X * 1024 *1024
    learnCfg.maxSteps       = 200;                                              // How many loops
    learnCfg.targetFitness  = 98.0f;                                            // What we are aiming for
    learnCfg.type           = LearnType::Language;                                  // The typ0e of learner

    ESConfig coachCfg;
    coachCfg.populationSize = 8;                                                // How big is this
    coachCfg.sigma          = 0.1f;                                             // sigma push
    coachCfg.decay          = 0.99f;                                            // decay rate
    coachCfg.envConfig      = learnCfg;                                         // the Learn Config

    ESCoach coach(ctx.pool, coachCfg);                                          // Create the Coach

    auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());                  // get the Language from the coach
    REQUIRE(llm != nullptr);

    const uint32_t inDim = llm->inputDimension();                              // Dim In
    const uint32_t outDim = llm->outputDimension();                            // Dim Out

    Genome bestGenome = buildTextLayers(inDim, outDim);                         // Create the Genome's with with the layers and also activation type
    std::string thought{};
    uint32_t gen = 0;
    JOB_LOG_INFO("[Motif Language] Start ...");
    while(!llm->done() && gen < learnCfg.maxSteps ) {
        bestGenome = coach.coach((gen == 0) ? bestGenome : coach.bestGenome()); // Evolve/train one generation save as survivor (which will later be the step fo non volitile)
        thought = llm->say(learnCfg.corpusLen, bestGenome);                    // Get the output of what it just learned (should be 14 ? )
        gen++;
    }

    JOB_LOG_INFO("[Motif Language] {} Final: {}", llm->fitness(),  thought.substr(0, 20));
    REQUIRE(llm->fitness() > 80.0f);
}



TEST_CASE("Reincarnation: Saving and Loading the LLM", "[ai][llm][io]")
{
    std::string vocabFile = "physics_vocab.bin";
    std::string brainFile = "physics_brain.bin";

    // Cleanup old files
    std::remove(vocabFile.c_str());
    std::remove(brainFile.c_str());

    // The First Life (Training)
    {
        JobStealerCtx ctx(8);
        auto learnCfg = LearnPresets::LanguageConfig(
            finalWords.c_str(),
            job::ai::token::TokenType::Motif
        );
        learnCfg.contextLen = 6;
        learnCfg.corpusLen  = (finalWords.length() - learnCfg.contextLen);
        learnCfg.maxSteps = 250; // Ensure convergence
        learnCfg.seed = 42;

        ESConfig coachCfg;
        coachCfg.populationSize = 8;
        coachCfg.sigma          = 0.1f;
        coachCfg.decay          = 0.99f;
        coachCfg.envConfig = learnCfg;

        ESCoach coach(ctx.pool, coachCfg);
        auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());

        // Train
        const uint32_t inDim = llm->inputDimension();
        const uint32_t outDim = llm->outputDimension();
        Genome genome = buildTextLayers(inDim, outDim);

        uint32_t gen = 0;
        while (!llm->done() && gen < learnCfg.maxSteps) {
            genome = coach.coach((gen==0) ? genome : coach.bestGenome());
            (void)llm->say(learnCfg.corpusLen, genome);
            gen++;
        }

        // Verify Life 1 worked
        REQUIRE(llm->fitness() > 90.0f);
        Genome survivorGenome = coach.bestGenome();

        // SAVE THE DICTIONARY
        std::ofstream vFile(vocabFile, std::ios::binary);
        REQUIRE(vFile.is_open());
        REQUIRE(llm->tokenizer()->save(vFile));
        vFile.close(); // Flush

        // SAVE THE BRAIN
        REQUIRE(GenomeSerializer::save(survivorGenome, brainFile));

        JOB_LOG_INFO("[Life 1] Saved llm to disk.");
    } // Language and Coach destroyed here

    // The Second Life (Inference Only)
    {
        JOB_LOG_INFO("[Life 2] Waking up llm from disk...");
        JobFifoCtx ctx(1);
        // Load Brain
        Genome reincarnatedGenome = GenomeSerializer::load(brainFile);
        REQUIRE(reincarnatedGenome.weights.size() > 0);
        REQUIRE(reincarnatedGenome.architecture.size() > 0);

        // Setup Learner (Simulate App Start)
        auto learnCfg = LearnPresets::LanguageConfig(finalWords.c_str(), job::ai::token::TokenType::Motif);
        learnCfg.contextLen = 6;
        learnCfg.corpusLen = (uint32_t)(finalWords.length() - learnCfg.contextLen);
        // Fresh Language, empty mind
        LanguageLearn newLanguage(learnCfg, ctx.pool);

        std::ifstream vFile(vocabFile, std::ios::binary);
        REQUIRE(vFile.is_open());
        REQUIRE(newLanguage.tokenizer()->load(vFile));
        vFile.close();

        // The new Language runs the old brain on the old dictionary.
        std::string thought = newLanguage.say(learnCfg.corpusLen, reincarnatedGenome);
        JOB_LOG_INFO("[Life 2] Recalled: {}", thought);

        // Check for success
        REQUIRE(thought.size() > 0);
        REQUIRE(thought.find("Joseph") != std::string::npos);
    }
}



#if 0

std::string loadFile(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
}



Genome buildSpecialLayers(uint32_t inputSize, uint32_t outputSize, int hidden = 32)
{
    Genome g;
    int kHidden = hidden;

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


TEST_CASE("Convo: Saving and Loading the LLM", "[ai][llm][io]")
{
    const std::string kChatCorpus =
        "<User>Who are you?\n"
        "<Bard>I am the Physics Bard.\n"
        "<User>What are you made of?\n"
       "<Bard>I am made of code and math.\n"
       "<User>Who built you?\n"
       "<Bard>I was built by Joseph.\n"
        "<User>Do you like C++?\n"
        "<Bard>I love C++ templates.\n"
        "<User>What is 1+1?\n"
        "<Bard>It is 2.\n"
        "<User>Who are you?\n"
        "<Bard>I am the Physics Bard.\n";

    std::string promptStr = "<User>Who are you?\n<Bard>";

    std::string vocabFile = "convo_vocab.bin";
    std::string brainFile = "convo_brain.bin";

    // Cleanup old files
    // std::remove(vocabFile.c_str());
    // std::remove(brainFile.c_str());

    // The First Life (Training)
    {
        JobFifoCtx ctx(24);
        auto learnCfg = LearnPresets::LanguageConfig(
            kChatCorpus.c_str(),
            job::ai::token::TokenType::Byte
            );
        learnCfg.contextLen = 256;//kChatCorpus.length();
        learnCfg.corpusLen  = (kChatCorpus.length() - learnCfg.contextLen);
        learnCfg.maxSteps = 5000; // Ensure convergence
        learnCfg.seed = 42;
        learnCfg.initWsMb = 4;
        learnCfg.targetFitness  = 97.0f;



        ESConfig coachCfg;
        coachCfg.populationSize = 64;
        coachCfg.sigma          = 0.08f;
        coachCfg.decay          = 0.995f;
        coachCfg.envConfig = learnCfg;

        ESCoach coach(ctx.pool, coachCfg);
        auto *llm = dynamic_cast<LanguageLearn*>(coach.learner());

        // Train
        const uint32_t inDim = llm->inputDimension();
        const uint32_t outDim = llm->outputDimension();
        Genome genome = buildSpecialLayers(inDim, outDim, 32);
        std::string thought{};

        uint32_t gen = 0;
        while (!llm->done() && gen < learnCfg.maxSteps) {
            genome = coach.coach((gen==0) ? genome : coach.bestGenome());
            thought = llm->say(learnCfg.corpusLen, genome);
            if(gen > 0 && (gen % 10 == 0))
                JOB_LOG_INFO("[Convo Model] Gen {}/{}  P {}  ||  weight {}  ||  fitness {} || {}",
                             gen,
                             learnCfg.maxSteps,
                             coachCfg.populationSize,
                             genome.weights[gen -1],
                             llm->fitness(),
                             thought);
            gen++;
        }

        // Verify Life 1 worked
        REQUIRE(llm->fitness() > 90.0f);
        /*
        Genome survivorGenome = coach.bestGenome();
        SAVE THE DICTIONARY
        // std::ofstream vFile(vocabFile, std::ios::binary);
        // REQUIRE(vFile.is_open());
        // REQUIRE(llm->tokenizer()->save(vFile));
        // vFile.close(); // Flush

        // SAVE THE BRAIN
        REQUIRE(GenomeSerializer::save(survivorGenome, brainFile));
        */
        JOB_LOG_INFO("[Convo Life 1] Saved llm to disk.");
    } // Language and Coach destroyed here
/*
    // The Shakespeare Second Life (Inference Only)
    {
        JOB_LOG_INFO("[Convo Life 2] Waking up llm from disk...");
        JobFifoCtx ctx(1);

        Genome reincarnatedGenome = GenomeSerializer::load(brainFile);
        REQUIRE(!reincarnatedGenome.weights.empty());
        REQUIRE(!reincarnatedGenome.architecture.empty());

        // Keep SAME token type + SAME contextLen as training
        std::string seed = promptStr;
        while (seed.size() < 64) seed += " "; // ensure enough bytes for contextLen=32

        auto learnCfg = LearnPresets::LanguageConfig(seed.c_str(), job::ai::token::TokenType::Motif);
        learnCfg.contextLen = 32;
        learnCfg.corpusLen  = uint32_t(seed.size() - learnCfg.contextLen);

        LanguageLearn newLanguage(learnCfg, ctx.pool);

        std::ifstream vFile(vocabFile, std::ios::binary);
        REQUIRE(vFile.is_open());
        REQUIRE(newLanguage.tokenizer()->load(vFile));
        vFile.close();

        // Ask for actual output bytes
        std::string thought = newLanguage.say(256, reincarnatedGenome);
        JOB_LOG_INFO("[Convo 2] Recalled: {}", thought);

        REQUIRE(thought.size() > 0);

    }
*/
}

#endif









