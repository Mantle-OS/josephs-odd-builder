#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>

#include <graph/gated_ffn_graph.h>
#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <weights/model_weights.h>

using namespace job::model;
using namespace job::ggml;

struct GatedFfnGraphFixture
{
    static constexpr int64_t kEmbeddingLength  = 32;
    static constexpr int64_t kIntermediateSize = 96;

    JobGgmlContext::UPtr context{JobGgmlContext::createUniqMetadata(128)};
    LayerWeights weights;

    GatedFfnGraphFixture()
    {
        REQUIRE(context != nullptr);
        REQUIRE(context->isValid());

        weights.ffnGate = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            kIntermediateSize);

        weights.ffnUp = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            kIntermediateSize);

        weights.ffnDown = context->newTensor2d(
            JobGgmlType::F32,
            kIntermediateSize,
            kEmbeddingLength);

        REQUIRE(weights.ffnGate != nullptr);
        REQUIRE(weights.ffnUp != nullptr);
        REQUIRE(weights.ffnDown != nullptr);

        REQUIRE(weights.ffnGate->isValid());
        REQUIRE(weights.ffnUp->isValid());
        REQUIRE(weights.ffnDown->isValid());
    }

    [[nodiscard]] JobGgmlTensorOp::UPtr input(uint32_t tokenCount)
    {
        auto tensor = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            tokenCount);

        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());

        return JobGgmlTensorOp::createUniq(
            tensor->tensor(),
            context.get());
    }

    void addBiases()
    {
        weights.ffnGateBias = context->newTensor1d(
            JobGgmlType::F32,
            kIntermediateSize);

        weights.ffnUpBias = context->newTensor1d(
            JobGgmlType::F32,
            kIntermediateSize);

        weights.ffnDownBias = context->newTensor1d(
            JobGgmlType::F32,
            kEmbeddingLength);

        REQUIRE(weights.ffnGateBias != nullptr);
        REQUIRE(weights.ffnUpBias != nullptr);
        REQUIRE(weights.ffnDownBias != nullptr);

        REQUIRE(weights.ffnGateBias->isValid());
        REQUIRE(weights.ffnUpBias->isValid());
        REQUIRE(weights.ffnDownBias->isValid());
    }
};

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("GatedFfnGraph builds a SiLU gated feed-forward graph", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(4);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("GatedFfnGraph builds a GELU gated feed-forward graph", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(4);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Gelu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("GatedFfnGraph builds a ReLU gated feed-forward graph", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(4);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Relu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("GatedFfnGraph defaults to SiLU", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(3);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 3);
}

TEST_CASE("GatedFfnGraph supports projection biases", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;
    fixture.addBiases();

    auto input = fixture.input(4);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("GatedFfnGraph preserves token count", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    for (uint32_t tokenCount : {1u, 2u, 4u, 8u}) {
        auto input = fixture.input(tokenCount);

        auto output = GatedFfnGraph::build(
            std::move(input),
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32);

        REQUIRE(output != nullptr);
        REQUIRE(output->isValid());

        CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
        CHECK(output->extent(1) == tokenCount);
    }
}

TEST_CASE("GatedFfnGraph defaults to SiLU and F16 input", "[model][graph][ffn][usage]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(3);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 3);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("GatedFfnGraph rejects a missing input", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    CHECK_THROWS_AS(
        GatedFfnGraph::build(
            nullptr,
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("GatedFfnGraph rejects a missing gate projection", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    fixture.weights.ffnGate.reset();

    auto input = fixture.input(1);

    CHECK_THROWS_AS(
        GatedFfnGraph::build(
            std::move(input),
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("GatedFfnGraph rejects a missing up projection", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    fixture.weights.ffnUp.reset();

    auto input = fixture.input(1);

    CHECK_THROWS_AS(
        GatedFfnGraph::build(
            std::move(input),
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("GatedFfnGraph rejects a missing down projection", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    fixture.weights.ffnDown.reset();

    auto input = fixture.input(1);

    CHECK_THROWS_AS(
        GatedFfnGraph::build(
            std::move(input),
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("GatedFfnGraph output returns to embedding geometry", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(7);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 7);
}

TEST_CASE("GatedFfnGraph supports a larger intermediate expansion", "[model][graph][ffn][edge]")
{
    static constexpr int64_t embeddingLength  = 32;
    static constexpr int64_t intermediateSize = 256;

    auto context = JobGgmlContext::createUniqMetadata(128);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    LayerWeights weights;

    weights.ffnGate = context->newTensor2d(
        JobGgmlType::F32,
        embeddingLength,
        intermediateSize);

    weights.ffnUp = context->newTensor2d(
        JobGgmlType::F32,
        embeddingLength,
        intermediateSize);

    weights.ffnDown = context->newTensor2d(
        JobGgmlType::F32,
        intermediateSize,
        embeddingLength);

    auto tensor = context->newTensor2d(
        JobGgmlType::F32,
        embeddingLength,
        4);

    REQUIRE(weights.ffnGate != nullptr);
    REQUIRE(weights.ffnUp != nullptr);
    REQUIRE(weights.ffnDown != nullptr);
    REQUIRE(tensor != nullptr);

    auto input = JobGgmlTensorOp::createUniq(
        tensor->tensor(),
        context.get());

    auto output = GatedFfnGraph::build(
        std::move(input),
        weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == embeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("GatedFfnGraph supports F16 input casting", "[model][graph][ffn][edge]")
{
    GatedFfnGraphFixture fixture;

    auto input = fixture.input(4);

    auto output = GatedFfnGraph::build(
        std::move(input),
        fixture.weights,
        GatedFfnGraph::Activation::Silu,
        JobGgmlType::F16);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == GatedFfnGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark GatedFfnGraph construction", "[model][graph][ffn][benchmark]")
{
    BENCHMARK("Build tiny SwiGLU FFN graph")
    {
        GatedFfnGraphFixture fixture;

        auto input = fixture.input(4);

        auto output = GatedFfnGraph::build(
            std::move(input),
            fixture.weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32);

        return output->isValid();
    };
}

TEST_CASE("Benchmark Qwen-sized FFN graph construction", "[model][graph][ffn][benchmark]")
{
    BENCHMARK("Build Qwen3 2560 -> 9728 -> 2560 FFN graph")
    {
        auto context = JobGgmlContext::createUniqMetadata(128);

        LayerWeights weights;

        weights.ffnGate = context->newTensor2d(
            JobGgmlType::F32,
            2560,
            9728);

        weights.ffnUp = context->newTensor2d(
            JobGgmlType::F32,
            2560,
            9728);

        weights.ffnDown = context->newTensor2d(
            JobGgmlType::F32,
            9728,
            2560);

        auto tensor = context->newTensor2d(
            JobGgmlType::F32,
            2560,
            4);

        auto input = JobGgmlTensorOp::createUniq(
            tensor->tensor(),
            context.get());

        auto output = GatedFfnGraph::build(
            std::move(input),
            weights,
            GatedFfnGraph::Activation::Silu,
            JobGgmlType::F32);

        return output->isValid();
    };
}

#endif