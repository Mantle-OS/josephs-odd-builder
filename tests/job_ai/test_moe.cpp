#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>
#include <algorithm>
#include <random>

#include <ctx/job_stealing_ctx.h>

#include <sparse_moe.h>
#include <dense.h>
#include <workspace.h>

using Catch::Approx;
using namespace job::ai;
using namespace job::ai::moe;
using namespace job::ai::layers;
using namespace job::threads;
using namespace job::ai::infer;

// Simple Mock Expert
class ScalarMultLayer : public AbstractLayer {
public:
    ScalarMultLayer(float scalar, LayerConfig cfg = LayerPresets::DenseConfig(), float alpha = 0.0f) :
        AbstractLayer{cfg, alpha},
        m_scalar(scalar)
    {
    }

    [[nodiscard]] LayerType type() const noexcept override
    {
        // Lie for test
        return LayerType::Dense;
    }

    cords::ViewR::Extent getOutputShape(const cords::ViewR::Extent &inputShape) const override
    {
        return inputShape;
    };

    void forward(ThreadPool&, const cords::ViewR &input, cords::ViewR &output, Workspace&) override
    {
        const size_t n = input.size();
        for(size_t i=0; i<n; ++i)
            output[i] = input[i] * m_scalar;
    }

    [[nodiscard]] cords::ViewR parameters() noexcept override
    {
        return cords::ViewR(nullptr, cords::ViewR::Extent{0});
    }
    [[nodiscard]] size_t parameterCount() const noexcept override
    {
        return 0;
    }

    void loadWeights(float *) override
    {

    }

    // the size of the scratch pad.
    [[nodiscard]] size_t scratchSize(const cords::ViewR::Extent &) const override
    {
        return 0;
    }

    // Reset any internal state (KV cache, running stats, RNG state, etc.). when I get here ....
    virtual void resetState() noexcept override {};





private:
    float           m_scalar;
    std::string     m_layerName{"ScalarMock"};

};


// examples
TEST_CASE("SparseMoE: Deterministic Top-1 Routing", "[ai][moe][usage]")
{
    JobStealerCtx ctx(8);
    // NEW: Create Workspace
    Workspace ws(16 * 1024 * 1024);

    // Input Dim 4, 2 Experts, Top-1
    const int dim = 4;
    const int numExperts = 2;
    SparseMoE moe(dim, numExperts, 1);

    // expert 0 multiplies by 10.0
    // expert 1 multiplies by -10.0
    moe.addExpert(0, std::make_shared<ScalarMultLayer>(10.0f));
    moe.addExpert(1, std::make_shared<ScalarMultLayer>(-10.0f));

    // "Rig" the Router
    auto gateParams = moe.parameters();
    float* w = gateParams.data();
    std::fill_n(w, gateParams.size(), 0.0f);

    // Set weights: Expert 0 loves positive, Expert 1 loves negative
    for(int d=0; d<dim; ++d) {
        w[d * numExperts + 0] = 1.0f;  // Expert 0 weight
        w[d * numExperts + 1] = -1.0f; // Expert 1 weight
    }

    // create batch
    std::vector<float> inputData = {
        1.f,  1.f,  1.f,  1.f,  // -> Expert 0
        -1.f, -1.f, -1.f, -1.f   // -> Expert 1
    };

    std::vector<float> outputData(8, 0.0f);

    cords::ViewR inView(inputData.data(), cords::ViewR::Extent{2, 4});
    cords::ViewR outView(outputData.data(), cords::ViewR::Extent{2, 4});

    // UPDATED: Pass Workspace
    moe.forward(*ctx.pool, inView, outView, ws);

    CHECK(outputData[0] == Approx(10.0f)); // 1.0 * 10.0
    CHECK(outputData[4] == Approx(10.0f)); // -1.0 * -10.0
}

TEST_CASE("SparseMoE: Top-2 Routing (Mixing Experts)", "[ai][moe][usage]")
{
    JobStealerCtx ctx(8);
    Workspace ws(16 * 1024 * 1024);

    // dim 4, 3 experts, top-2
    SparseMoE moe(4, 3, 2);

    moe.addExpert(0, std::make_shared<ScalarMultLayer>(1.0f));
    moe.addExpert(1, std::make_shared<ScalarMultLayer>(2.0f));
    moe.addExpert(2, std::make_shared<ScalarMultLayer>(3.0f));

    // "Rig" weights so input goes to e0 and e1
    auto gateParams = moe.parameters();
    float* w = gateParams.data();
    std::fill_n(w, gateParams.size(), 0.0f);

    for(int d=0; d<4; ++d) {
        w[d * 3 + 0] = 10.0f;
        w[d * 3 + 1] = 10.0f;
        w[d * 3 + 2] = -10.0f;
    }

    std::vector<float> inputData(4, 1.0f);
    std::vector<float> outputData(4, 0.0f);

    cords::ViewR inView(inputData.data(), cords::ViewR::Extent{1, 4});
    cords::ViewR outView(outputData.data(), cords::ViewR::Extent{1, 4});

    moe.forward(*ctx.pool, inView, outView, ws);

    // output = (input * 1.0 * 0.5) + (input * 2.0 * 0.5) = 1.5
    CHECK(outputData[0] == Approx(1.5f).margin(0.01f));
}


// edge cases
TEST_CASE("SparseMoE: Empty Input Batch", "[ai][moe][edge]") {
    JobStealerCtx ctx(4);
    Workspace ws(1024 * 1024);
    SparseMoE moe(16, 4, 1);

    cords::ViewR inView(nullptr, cords::ViewR::Extent{0, 16});
    cords::ViewR outView(nullptr, cords::ViewR::Extent{0, 16});

    // should not crash
    moe.forward(*ctx.pool, inView, outView, ws);
}

TEST_CASE("SparseMoE: Missing Expert Implementation", "[ai][moe][edge]") {
    JobStealerCtx ctx(4);
    Workspace ws(1024 * 1024);
    SparseMoE moe(4, 2, 1);

    // add expert 0, but leave expert 1 null
    moe.addExpert(0, std::make_shared<ScalarMultLayer>(1.0f));

    auto gateParams = moe.parameters();
    float* w = gateParams.data();
    for(int d=0; d<4; ++d) {
        w[d * 2 + 0] = -10.0f;
        w[d * 2 + 1] = 10.0f;
    }

    std::vector<float> input(4, 1.0f);
    std::vector<float> output(4, 999.0f);

    cords::ViewR inView(input.data(), cords::ViewR::Extent{1, 4});
    cords::ViewR outView(output.data(), cords::ViewR::Extent{1, 4});

    // "Should" route to E1, see it's null, and do nothing (result 0 due to memset)
    moe.forward(*ctx.pool, inView, outView, ws);

    CHECK(output[0] == 0.0f);
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("SparseMoE: Throughput Benchmark (LLaMA-Small Scale)", "[ai][moe][bench]") {
    // simulating a small moe layer
    const int B = 128;
    const int D = 1024;
    const int E = 8;
    const int K = 2;

    JobStealerCtx ctx(16); // me cave man me give THREADS!
    Workspace ws(256 * 1024 * 1024); // Give it a real workspace (256MB)

    SparseMoE moe(D, E, K);

    // auto den_cfg = LayerPresets::DenseConfig();
    // den_cfg.activation = comp::ActivationType::GELU;

    // populate experts with real dense layers
    for(int i = 0; i < E; ++i)
        moe.addExpert(i, std::make_shared<DenseLayer>(D, D /*, den_cfg*/));

    using Alloc = cords::AlignedAllocator<float, 64>;
    std::vector<float, Alloc> input(B * D, 0.1f);
    std::vector<float, Alloc> output(B * D);

    cords::ViewR inView(input.data(),
                        cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(D)});

    cords::ViewR outView(output.data(),
                         cords::ViewR::Extent{static_cast<uint32_t>(B), static_cast<uint32_t>(D)});

    BENCHMARK("MoE Forward (128x1024, 8 Experts, Top2)") {
        moe.forward(*ctx.pool, inView, outView, ws);
        return output[0];
    };
}
#endif
