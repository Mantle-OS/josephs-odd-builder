#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <limits>

#include <ggml-cpu.h>

#include <config/model_config.h>
#include <graph/attention_graph.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <kv/kv_cache.h>
#include <weights/model_weights.h>

using namespace job::model;
using namespace job::ggml;

struct AttentionGraphFixture
{
    static constexpr uint32_t kEmbeddingLength = 32;
    static constexpr uint32_t kHeadCount       = 4;
    static constexpr uint32_t kHeadCountKv     = 2;
    static constexpr uint32_t kHeadDimension   = 8;
    static constexpr uint32_t kContextLength   = 16;
    static constexpr uint32_t kLayerCount      = 2;

    ModelConfig config{makeConfig()};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};
    KvCache kvCache{config, bufferType, kContextLength, JobGgmlType::F32};

    JobGgmlContext::UPtr context{JobGgmlContext::createUniqMetadata(512)};
    LayerWeights weights;

    AttentionGraphFixture()
    {
        REQUIRE(context != nullptr);
        REQUIRE(context->isValid());

        weights.layerIndex = 0;

        weights.attnQ = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            kHeadCount * kHeadDimension);

        weights.attnK = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            kHeadCountKv * kHeadDimension);

        weights.attnV = context->newTensor2d(
            JobGgmlType::F32,
            kEmbeddingLength,
            kHeadCountKv * kHeadDimension);

        weights.attnOut = context->newTensor2d(
            JobGgmlType::F32,
            kHeadCount * kHeadDimension,
            kEmbeddingLength);

        REQUIRE(weights.attnQ != nullptr);
        REQUIRE(weights.attnK != nullptr);
        REQUIRE(weights.attnV != nullptr);
        REQUIRE(weights.attnOut != nullptr);

        REQUIRE(weights.attnQ->isValid());
        REQUIRE(weights.attnK->isValid());
        REQUIRE(weights.attnV->isValid());
        REQUIRE(weights.attnOut->isValid());
    }

    [[nodiscard]] static ModelConfig makeConfig()
    {
        ModelConfig config;

        auto &transformer = config.transformerConfig();
        transformer.setContextLength(kContextLength);
        transformer.setEmbeddingLength(kEmbeddingLength);
        transformer.setBlockCount(kLayerCount);
        transformer.setVocabSize(128);

        auto &attention = config.attentionConfig();
        attention.setHeadCount(kHeadCount);
        attention.setHeadCountKv(kHeadCountKv);
        attention.setKeyLength(kHeadDimension);
        attention.setValueLength(kHeadDimension);
        attention.setSlidingWindowSize(0);
        attention.setAttnLogitSoftCapping(0.0f);

        return config;
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

    [[nodiscard]] JobGgmlTensor::UPtr positions(uint32_t tokenCount)
    {
        auto tensor = context->newTensor1d(
            JobGgmlType::I32,
            tokenCount);

        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());

        return tensor;
    }

    void addAttentionBiases()
    {
        weights.attnQBias = context->newTensor1d(
            JobGgmlType::F32,
            kHeadCount * kHeadDimension);

        weights.attnKBias = context->newTensor1d(
            JobGgmlType::F32,
            kHeadCountKv * kHeadDimension);

        weights.attnVBias = context->newTensor1d(
            JobGgmlType::F32,
            kHeadCountKv * kHeadDimension);

        weights.attnOutBias = context->newTensor1d(
            JobGgmlType::F32,
            kEmbeddingLength);

        REQUIRE(weights.attnQBias != nullptr);
        REQUIRE(weights.attnKBias != nullptr);
        REQUIRE(weights.attnVBias != nullptr);
        REQUIRE(weights.attnOutBias != nullptr);
    }

    void addQkNorms()
    {
        weights.attnQNorm = context->newTensor1d(
            JobGgmlType::F32,
            kHeadDimension);

        weights.attnKNorm = context->newTensor1d(
            JobGgmlType::F32,
            kHeadDimension);

        REQUIRE(weights.attnQNorm != nullptr);
        REQUIRE(weights.attnKNorm != nullptr);
    }
};

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("AttentionGraph builds grouped-query self attention", "[model][graph][attention][usage]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(4);
    auto positions = fixture.positions(4);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        0,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 4);
}

TEST_CASE("AttentionGraph builds attention with an existing KV prefix", "[model][graph][attention][usage]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(3);
    auto positions = fixture.positions(3);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        1,
        5,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 3);
}

TEST_CASE("AttentionGraph supports projection biases", "[model][graph][attention][usage]")
{
    AttentionGraphFixture fixture;
    fixture.addAttentionBiases();

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        0,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 2);
}

TEST_CASE("AttentionGraph supports Q and K normalization", "[model][graph][attention][usage]")
{
    AttentionGraphFixture fixture;
    fixture.addQkNorms();

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        0,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 2);
}

TEST_CASE("AttentionGraph supports attention logit soft capping", "[model][graph][attention][usage]")
{
    AttentionGraphFixture fixture;
    fixture.config.attentionConfig().setAttnLogitSoftCapping(30.0f);

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        0,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 2);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("AttentionGraph rejects a missing input", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;
    auto positions = fixture.positions(1);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            nullptr,
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph rejects invalid attention configuration", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    fixture.config.attentionConfig().setHeadCount(0);

    auto input = fixture.input(1);
    auto positions = fixture.positions(1);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph requires I32 position tensors", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(2);

    auto positions = fixture.context->newTensor1d(
        JobGgmlType::F32,
        2);

    REQUIRE(positions != nullptr);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph requires one position per input token", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(4);
    auto positions = fixture.positions(3);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph rejects missing required projections", "[model][graph][attention][edge]")
{
    SECTION("Q projection")
    {
        AttentionGraphFixture fixture;
        fixture.weights.attnQ.reset();

        auto input = fixture.input(1);
        auto positions = fixture.positions(1);

        CHECK_THROWS_AS(
            AttentionGraph::build(
                std::move(input),
                fixture.weights,
                fixture.config.attentionConfig(),
                fixture.kvCache,
                *positions,
                0,
                0,
                1e-6f,
                AttentionGraphFixture::kHeadDimension),
            std::invalid_argument);
    }

    SECTION("K projection")
    {
        AttentionGraphFixture fixture;
        fixture.weights.attnK.reset();

        auto input = fixture.input(1);
        auto positions = fixture.positions(1);

        CHECK_THROWS_AS(
            AttentionGraph::build(
                std::move(input),
                fixture.weights,
                fixture.config.attentionConfig(),
                fixture.kvCache,
                *positions,
                0,
                0,
                1e-6f,
                AttentionGraphFixture::kHeadDimension),
            std::invalid_argument);
    }

    SECTION("V projection")
    {
        AttentionGraphFixture fixture;
        fixture.weights.attnV.reset();

        auto input = fixture.input(1);
        auto positions = fixture.positions(1);

        CHECK_THROWS_AS(
            AttentionGraph::build(
                std::move(input),
                fixture.weights,
                fixture.config.attentionConfig(),
                fixture.kvCache,
                *positions,
                0,
                0,
                1e-6f,
                AttentionGraphFixture::kHeadDimension),
            std::invalid_argument);
    }

    SECTION("output projection")
    {
        AttentionGraphFixture fixture;
        fixture.weights.attnOut.reset();

        auto input = fixture.input(1);
        auto positions = fixture.positions(1);

        CHECK_THROWS_AS(
            AttentionGraph::build(
                std::move(input),
                fixture.weights,
                fixture.config.attentionConfig(),
                fixture.kvCache,
                *positions,
                0,
                0,
                1e-6f,
                AttentionGraphFixture::kHeadDimension),
            std::invalid_argument);
    }
}

TEST_CASE("AttentionGraph rejects a layer outside the KV cache", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(1);
    auto positions = fixture.positions(1);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            fixture.kvCache.layerCount(),
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::out_of_range);
}

TEST_CASE("AttentionGraph rejects nPast outside the GGML integer range", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(1);
    auto positions = fixture.positions(1);

    const uint32_t tooLarge =
        static_cast<uint32_t>(std::numeric_limits<int>::max()) + 1u;

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            tooLarge,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph rejects unsupported sliding-window attention", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    fixture.config.attentionConfig().setSlidingWindowSize(8);

    auto input = fixture.input(1);
    auto positions = fixture.positions(1);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            0,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::invalid_argument);
}

TEST_CASE("AttentionGraph rejects requests beyond KV cache capacity", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    CHECK_THROWS_AS(
        AttentionGraph::build(
            std::move(input),
            fixture.weights,
            fixture.config.attentionConfig(),
            fixture.kvCache,
            *positions,
            0,
            AttentionGraphFixture::kContextLength - 1,
            1e-6f,
            AttentionGraphFixture::kHeadDimension),
        std::out_of_range);
}

TEST_CASE("AttentionGraph accepts a request ending exactly at KV cache capacity", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        AttentionGraphFixture::kContextLength - 2,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    CHECK(output->isValid());
}

TEST_CASE("AttentionGraph derives head dimensions from embedding geometry", "[model][graph][attention][edge]")
{
    AttentionGraphFixture fixture;

    fixture.config.attentionConfig().setKeyLength(0);
    fixture.config.attentionConfig().setValueLength(0);

    REQUIRE(fixture.config.attentionConfig().isValid());

    auto input = fixture.input(2);
    auto positions = fixture.positions(2);

    auto output = AttentionGraph::build(
        std::move(input),
        fixture.weights,
        fixture.config.attentionConfig(),
        fixture.kvCache,
        *positions,
        0,
        0,
        1e-6f,
        AttentionGraphFixture::kHeadDimension,
        2,
        JobGgmlType::F32);

    REQUIRE(output != nullptr);
    REQUIRE(output->isValid());

    CHECK(output->extent(0) == AttentionGraphFixture::kEmbeddingLength);
    CHECK(output->extent(1) == 2);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark AttentionGraph construction", "[model][graph][attention][benchmark]")
{
    BENCHMARK("Build tiny GQA self-attention graph")
    {
        AttentionGraphFixture fixture;

        auto input = fixture.input(4);
        auto positions = fixture.positions(4);

        auto output = AttentionGraph::build(std::move(input),
                                            fixture.weights,
                                            fixture.config.attentionConfig(),
                                            fixture.kvCache,
                                            *positions,
                                            0,
                                            0,
                                            1e-6f,
                                            AttentionGraphFixture::kHeadDimension,
                                            2,
                                            JobGgmlType::F32);

        return output->isValid();
    };
}

#endif