#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <vector>
#include <iostream>
#include <algorithm>
#include <random> // Added for real randomness

#include <job_thread_pool.h>
#include <sched/job_work_stealing_scheduler.h>
#include <ctx/job_stealing_ctx.h>

#include <attention.h>

using namespace job::ai;
using namespace job::threads;


void randomize_buffer(float* ptr, size_t count) {
    static std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for(size_t i=0; i<count; ++i) {
        ptr[i] = dist(gen);
    }
}

void randomize_layer_params(layers::ILayer& layer) {
    auto params = layer.parameters();
    randomize_buffer(params.data(), params.extent()[0]);
}

TEST_CASE("Attention: Multi-Backend Integration", "[ai][attn][integration]") {

    // Setup Context
    JobStealerCtx ctx(8);

    const int B = 2;   // Small batch
    const int S = 128; // Sequence
    const int D = 64;  // Dim

    // 1. Randomize Inputs (Critical for Physics/FMM)
    // If these are all 1.0f, all particles overlap -> Zero Gravity.
    std::vector<float> inputData(B * S * D);
    randomize_buffer(inputData.data(), inputData.size());

    std::vector<float> outputData(B * S * D, 0.0f);

    cords::ViewR input(inputData.data(), cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(S)});
    cords::ViewR output(outputData.data(), cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(S)});

    // FMM
    SECTION("FMM Attention (O(N))") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::FMM;
        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);
        attn.forward(*ctx.pool, input, output);
        bool gotSignal = false;
        for(int i = 0; i < 10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }
    // 2. Dense
    SECTION("Dense Attention") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::Dense;

        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output);

        bool gotSignal = false;
        for(int i=0; i<10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }

    // 3. Verlet (Dynamics)
    SECTION("Verlet Dynamics (Particle Simulation)") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::Verlet;

        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);

        attn.forward(*ctx.pool, input, output);
        CHECK(outputData[0] != 0.0f);
    }


    // 4. Barns n Hut (Dynamics)
    SECTION("Barns and Hut Dynamics (Particle Simulation)") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::BarnesHut;

        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);

        attn.forward(*ctx.pool, input, output);
        CHECK(outputData[0] != 0.0f);
    }

    // 5. LowRank (Dynamics)
    SECTION("LowRank (Simulation)") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::LowRank;

        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output);

        bool gotSignal = false;
        for(int i=0; i<10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }

    // 5. Flash (Dynamics)
    SECTION("Flash (Simulation)") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::Flash;

        layers::Attention attn(cfg, D);
        randomize_layer_params(attn);

        // Clear output
        std::fill(outputData.begin(), outputData.end(), 0.0f);

        attn.forward(*ctx.pool, input, output);

        bool gotSignal = false;
        for(int i=0; i<10; ++i) {
            if (std::abs(outputData[i]) > 1e-5f) {
                gotSignal = true;
                break;
            }
        }
        CHECK(gotSignal);
    }
}


TEST_CASE("Attention: Scaling Benchmark (Seq=4096)", "[ai][attn][bench][scale]") {
    // Large Context. This is where O(N^2) dies and O(N) shines.
    const int B = 1; // Single batch to focus on Sequence Length scaling
    const int S = 4096;
    const int D = 64;

    JobStealerCtx ctx(16);

    std::vector<float> inputData(B * S * D);
    std::vector<float> outputData(B * S * D);
    randomize_buffer(inputData.data(), inputData.size());

    cords::ViewR input(inputData.data(), cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(S)});
    cords::ViewR output(outputData.data(), cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(S)});

    // Only run the scalable ones to save time in test suite
    BENCHMARK("FMM Attention (O(N)) - Long Context") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::FMM;
        layers::Attention attn(cfg, D);
        attn.forward(*ctx.pool, input, output);
        return outputData[0];
    };

    BENCHMARK("LowRank Attention (O(N)) - Long Context") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::LowRank;
        layers::Attention attn(cfg, D);
        attn.forward(*ctx.pool, input, output);
        return outputData[0];
    };

    BENCHMARK("Dense Attention - Long Context") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::Dense;
        layers::Attention attn(cfg, D);
        attn.forward(*ctx.pool, input, output);
        return outputData[0];
    };

    /*
    BENCHMARK("Flash Attention - Long Context") {
        layers::AttentionConfig cfg;
        cfg.adapterType = adapters::AdapterType::Flash;
        layers::Attention attn(cfg, D);
        attn.forward(*ctx.pool, input, output);
        return outputData[0];
    };
    */
}


