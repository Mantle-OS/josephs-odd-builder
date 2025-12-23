#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <vector>
#include <algorithm>
#include <random> // Added for real randomness

#include <job_thread_pool.h>
#include <sched/job_work_stealing_scheduler.h>
#include <ctx/job_stealing_ctx.h>

#include <attention.h>

using namespace job::ai;
using namespace job::threads;

void randomize_buffer(float* ptr, size_t count)
{
    static std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (size_t i = 0; i < count; ++i)
        ptr[i] = dist(gen);
}

void randomize_layer_params(layers::AbstractLayer &layer)
{
    auto params = layer.parameters();
    randomize_buffer(params.data(), params.extent().volume());
}

TEST_CASE("Attention: Multi-Backend Integration", "[ai][attn][integration]") {

    // Setup Context
    JobStealerCtx ctx(8);
    infer::Workspace ws(256 * 1024 * 1024);

    const uint32_t B = 2;   // Small batch
    const uint32_t S = 128; // Sequence
    const uint32_t D = 64;  // Dim

    // 1. Randomize Inputs (Critical for Physics/FMM)
    // If these are all 1.0f, all particles overlap -> Zero Gravity.
    std::vector<float> inputData(static_cast<size_t>(B) * S * D);
    randomize_buffer(inputData.data(), inputData.size());

    std::vector<float> outputData(static_cast<size_t>(B) * S * D, 0.0f);

    cords::ViewR input(
        inputData.data(),
        cords::makeBSD(B, S, D)
        );

    cords::ViewR output(
        outputData.data(),
        cords::makeBSD(B, S, D)
        );

    // 1. FMM
    SECTION("FMM Attention (O(N))") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::FMM;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        attn.forward(*ctx.pool, input, output, ws);

        bool gotSignal = false;
        for (int i = 0; i < 10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }

    // 2. Dense
    SECTION("Dense Attention") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::Dense;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output, ws);

        bool gotSignal = false;
        for (int i = 0; i < 10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }

    // 3. Verlet (Dynamics)
    SECTION("Verlet Dynamics (Particle Simulation)") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::Verlet;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        attn.forward(*ctx.pool, input, output, ws);
        CHECK(outputData[0] != 0.0f);
    }

    // 4. Barnes-Hut (Dynamics)
    SECTION("Barns and Hut Dynamics (Particle Simulation)") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::BarnesHut;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        attn.forward(*ctx.pool, input, output, ws);
        CHECK(outputData[0] != 0.0f); // REGRESSION PROBE
    }

    // 5. LowRank (Dynamics)
    SECTION("LowRank (Simulation)") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::LowRank;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output, ws);

        bool gotSignal = false;
        for (int i = 0; i < 10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }

    // 6. Flash (Dynamics)
    SECTION("Flash (Simulation)") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::Flash;

        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output, ws);

        bool gotSignal = false;
        for (int i = 0; i < 10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Attention: Scaling Benchmark (Seq=4096)", "[ai][attn][bench][scale]") {
    // Large Context. This is where O(N^2) dies and O(N) shines.
    const uint32_t B = 1; // Single batch to focus on Sequence Length scaling
    const uint32_t S = 4096;
    const uint32_t D = 64;

    JobStealerCtx ctx(16);

    infer::Workspace ws(512 * 1024 * 1024); // Bigger workspace for benchmarks
    std::vector<float> inputData(static_cast<size_t>(B) * S * D);
    std::vector<float> outputData(static_cast<size_t>(B) * S * D);
    randomize_buffer(inputData.data(), inputData.size());

    cords::ViewR input(
        inputData.data(),
        cords::makeBSD(B, S, D)
        );
    cords::ViewR output(
        outputData.data(),
        cords::makeBSD(B, S, D)
        );

    // Only run the scalable ones to save time in test suite
    BENCHMARK("FMM Attention (O(N) Seq=4096)) - Long Context") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::FMM;
        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        attn.forward(*ctx.pool, input, output, ws);
        return outputData[0];
    };

    BENCHMARK("LowRank Attention (O(N) Seq=4096)) - Long Context") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::LowRank;
        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        attn.forward(*ctx.pool, input, output, ws);
        return outputData[0];
    };

    BENCHMARK("Dense Attention Seq=4096) - Long Context") {
        layers::LayerConfig cfg;
        cfg.adapterType = adapters::AdapterType::Dense;
        layers::AttentionLayer attn(static_cast<int>(D), cfg);
        attn.forward(*ctx.pool, input, output, ws);
        return outputData[0];
    };

}
#endif
