#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <random>

#include <ctx/job_stealing_ctx.h>

#include <genome.h>
#include <activation_types.h>
#include <layer_types.h>
#include <adapter_types.h>
#include <router_types.h>
#include <layers/attention.h>

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

void addAttentionGene(Genome &genome, int dim, int heads, adapters::AdapterType type = adapters::AdapterType::Flash) {
    LayerGene gene{};
    gene.type = layers::LayerType::Attention;
    gene.inputs = dim;
    gene.outputs = heads;
    gene.auxiliaryData = static_cast<uint32_t>(type); // Use Flash

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

TEST_CASE("Runner: End-to-End Pipeline Smoke Test 4 threads", "[runner][integration]") {
    JobStealerCtx ctx(4);

    Genome genome;
    // architecture: input(64) -> dense(128) -> attention(128) -> moe(128) -> dense(10) -> output
    const int D = 64;
    const int Hidden = 128;
    const int Classes = 10;

    addDenseGene(genome, D, Hidden, comp::ActivationType::Swish);
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::Flash);              // 4 heads
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::LowRank);            // 4 heads
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::FMM);                // 4 heads
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::BarnesHut);          // 4 heads
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::Verlet);             // 4 heads
    addAttentionGene(genome, Hidden, 4, adapters::AdapterType::Stencil);            // 4 heads
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

TEST_CASE("Runner: Genome Ownership Safety 4 threads", "[runner][safety]") {
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


#ifndef JOB_CI_BUILD
#ifdef JOB_TEST_BENCHMARKS
static void randomizeAttBuffer(float *ptr, size_t count)
{
    static std::mt19937 gen(123);
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (size_t i = 0; i < count; ++i)
        ptr[i] = dist(gen);
}

static void randomizeLayerParams(layers::AbstractLayer &layer)
{
    auto p = layer.parameters();
    randomizeAttBuffer(p.data(), p.extent().volume());
}


TEST_CASE("Runner Bench: Attention adapters scaling", "[bench][attn]") {
    JobStealerCtx ctx(16);

    const uint32_t B = 1;
    const uint32_t D = 128;

    // Two regimes: short and long context
    const uint32_t S_short = 256;
    const uint32_t S_long  = 4096;
    const uint32_t SV_long  = 16384;

    auto run_one = [&](uint32_t S, adapters::AdapterType type, const char* name) {
        infer::Workspace ws(1024 * 1024 * 1024); // 1GB scratch (big enough for dense @ 4096)
        std::vector<float> in(size_t(B) * S * D);
        std::vector<float> out(size_t(B) * S * D);

        randomizeAttBuffer(in.data(), in.size());

        cords::ViewR input(in.data(), cords::makeBSD(B, S, D));
        cords::ViewR output(out.data(), cords::makeBSD(B, S, D));

        layers::LayerConfig cfg = layers::LayerPresets::AttentionConfig();
        cfg.adapterType = type;
        cfg.numHeads = 8;

        layers::AttentionLayer attn(int(D), cfg);
        randomizeLayerParams(attn);

        BENCHMARK(name) {
            attn.forward(*ctx.pool, input, output, ws);
            return output.data()[0];
        };
    };

    SECTION("Short context (S=256)") {
        run_one(S_short, adapters::AdapterType::Dense,     "Runner Attn Dense  S=256");
        run_one(S_short, adapters::AdapterType::Flash,     "Runner Attn Flash  S=256");
        run_one(S_short, adapters::AdapterType::LowRank,   "Runner Attn LowRank S=256");
        run_one(S_short, adapters::AdapterType::FMM,       "Runner Attn FMM    S=256");
        run_one(S_short, adapters::AdapterType::BarnesHut, "Runner Attn BarnesHut S=256");
        run_one(S_short, adapters::AdapterType::Verlet,    "Runner Attn Verlet S=256");
    }

    SECTION("Long context (S=4096)") {
        // Dense/Flash may be slow or memory-heavy here; keep it if you want the “dies here” reference point.
        run_one(S_long,  adapters::AdapterType::LowRank,   "Runner Attn LowRank S=4096");
        run_one(S_long,  adapters::AdapterType::FMM,       "Runner Attn FMM    S=4096");
        run_one(S_long,  adapters::AdapterType::BarnesHut, "Runner Attn BarnesHut S=4096");

        // Optional “reference cliff”
        // run_one(S_long,  adapters::AdapterType::Verlet,    "Runner Attn Verlet S=4096");
        // run_one(S_long, AdapterType::Dense,   "Attn Dense  S=4096 (O(N^2) ref)");
        // run_one(S_long, AdapterType::Flash,   "Attn Flash  S=4096 (ref)");
    }


    SECTION("Long context (S=16384)") {
        run_one(SV_long,  adapters::AdapterType::FMM,       "Runner Attn FMM S=16384");
        // run_one(SV_long,  adapters::AdapterType::BarnesHut, "Runner Attn BarnesHut S=16384");
    }

}
#endif
#endif



