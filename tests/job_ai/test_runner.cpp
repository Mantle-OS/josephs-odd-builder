#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>
#include <random>
#include <numeric>

#include <ctx/job_stealing_ctx.h>

#include <genome.h>
#include <activation_types.h>
#include <layer_types.h>
#include <adapter_types.h>
#include <router_types.h>

#include <runner.h>

using namespace job::ai;
using namespace job::ai::infer;
using namespace job::ai::evo;
using namespace job::threads;

void addDenseGene(Genome &genome, int input, int output, comp::ActivationType act)
{
    LayerGene gene{};
    gene.type = layers::LayerType::Dense;
    gene.inputs = input;
    gene.outputs = output;
    gene.activation = act;

    // weights + bias
    size_t count = static_cast<size_t>(input) * output + output;

    gene.weightOffset = static_cast<uint32_t>(genome.weights.size());
    gene.weightCount = static_cast<uint32_t>(count);

    genome.architecture.push_back(gene);

    static std::mt19937 gen(42);
    static std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    for(size_t i=0; i<count; ++i)
        genome.weights.push_back(dist(gen));
}

void addAttentionGene(Genome &genome, int dim, int heads) {
    LayerGene gene{};
    gene.type = layers::LayerType::Attention;
    gene.inputs = dim;
    gene.outputs = heads;
    gene.auxiliaryData = static_cast<uint32_t>(adapters::AdapterType::Flash); // Use Flash

    // 4 matrices (Q,K,V,O) + bias's
    size_t count = 4 * (size_t(dim) * dim) + 4 * dim;

    gene.weightOffset = static_cast<uint32_t>(genome.weights.size());
    gene.weightCount = static_cast<uint32_t>(count);

    genome.architecture.push_back(gene);

    static std::mt19937 gen(42);
    static std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    for(size_t i=0; i<count; ++i)
        genome.weights.push_back(dist(gen));
}

void addMoEGene(Genome &genome, int dim, int experts, int topk)
{
    LayerGene gene{};
    gene.type = layers::LayerType::SparseMoE;
    gene.inputs = dim;
    gene.outputs = experts;
    gene.auxiliaryData = topk;
    gene.activation = comp::ActivationType::Identity;

    size_t gateCount = static_cast<size_t>(dim) * experts;

    gene.weightOffset = static_cast<uint32_t>(genome.weights.size());
    gene.weightCount = static_cast<uint32_t>(gateCount);

    genome.architecture.push_back(gene);

    static std::mt19937 gen(42);
    static std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    for(size_t i=0; i<gateCount; ++i)
        genome.weights.push_back(dist(gen));
}

TEST_CASE("Runner: End-to-End Pipeline Smoke Test", "[runner][integration]") {
    JobStealerCtx ctx(8);

    Genome genome;
    // architecture: input(64) -> dense(128) -> attention(128) -> moe(128) -> dense(10) -> output
    const int D = 64;
    const int Hidden = 128;
    const int Classes = 10;

    addDenseGene(genome, D, Hidden, comp::ActivationType::Swish);
    addAttentionGene(genome, Hidden, 4); // 4 heads
    addMoEGene(genome, Hidden, 8, 2);    // 8 experts, top-2
    addDenseGene(genome, Hidden, Classes, comp::ActivationType::Identity);

    // this triggers layerfactory, weight loading, and workspace allocation
    Runner runner(genome, ctx.pool);

    // batch=4, seq=16, dim=64
    const int B = 4;
    const int S = 16;

    std::vector<float> inputData(B * S * D);
    std::fill(inputData.begin(), inputData.end(), 0.5f);
    cords::ViewR inputView(inputData.data(),
                           cords::ViewR::Extent{
                               static_cast<uint32_t>(B * S), // Rows: 64
                               static_cast<uint32_t>(D)      // Cols: 64
                           }
                           );

    cords::ViewR output = runner.run(inputView);

    // check for nans or infinities (the silent killers)
    REQUIRE(output.extent()[0] == B * S);
    REQUIRE(output.extent()[1] == Classes);

    const float *data = output.data();
    size_t size = output.size();
    bool sane = true;

    for(size_t i=0; i<size; ++i) {
        uint32_t bits;
        std::memcpy(&bits, &data[i], 4);

        // Mask out the exponent bits (0x7F800000).
        // if they are all 1s, it is either infinity or nan.
        bool is_bad = (bits & 0x7F800000) == 0x7F800000;

        if (is_bad) {
            sane = false;
            JOB_LOG_ERROR("Network exploded at index {}", i);
            break;
        }
    }
    REQUIRE(sane);
}

TEST_CASE("Runner: Genome Ownership Safety", "[runner][safety]") {
    JobStealerCtx ctx(4);
    std::unique_ptr<Runner> runner;

    {
        // scope to destroy genome
        Genome tempGenome;
        addDenseGene(tempGenome, 16, 16, comp::ActivationType::ReLU);
        runner = std::make_unique<Runner>(tempGenome, ctx.pool);
        // tempGenome dies here
    }

    // runner should still function because it copied the genome
    std::vector<float> input(16, 1.0f);
    cords::ViewR inView(input.data(), cords::ViewR::Extent{1, 16});

    cords::ViewR out = runner->run(inView);
    REQUIRE(out.size() == 16);
}
