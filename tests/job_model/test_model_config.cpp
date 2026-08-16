#include <catch2/catch_test_macros.hpp>

#include <real_type.h>

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <config/model_config.h>

using namespace job::model;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("ModelConfig composes a transformer configuration", "[model][config][usage]")
{
    ModelConfig config;

    config.archConfig().setArch(ModelArchitecture::Llama);
    config.archConfig().setArchName("llama");
    config.archConfig().setModelName("Llama-3-8B-Instruct");
    config.archConfig().setHiddenActivation("silu");

    config.transformerConfig().setContextLength(8192);
    config.transformerConfig().setEmbeddingLength(4096);
    config.transformerConfig().setBlockCount(32);
    config.transformerConfig().setVocabSize(128256);

    config.attentionConfig().setHeadCount(32);
    config.attentionConfig().setHeadCountKv(8);
    config.attentionConfig().setKeyLength(128);
    config.attentionConfig().setValueLength(128);

    config.normConfig().setRmsNormEps(1e-5f);

    config.ropeConfig().setRopeDimensionCount(128);
    config.ropeConfig().setRopeFreqBase(500000.0f);

    config.feedForwardConfig().setFeedForwardLength(14336);

    REQUIRE(config.isValid());

    CHECK(config.architectureName() == "llama");
    CHECK(config.transformerConfig().contextLength() == 8192);
    CHECK(config.transformerConfig().embeddingLength() == 4096);
    CHECK(config.transformerConfig().blockCount() == 32);
    CHECK(config.transformerConfig().vocabSize() == 128256);

    CHECK(config.attentionConfig().headCount() == 32);
    CHECK(config.attentionConfig().headCountKv() == 8);
    CHECK(config.attentionConfig().headDimension(4096) == 128);
    CHECK(config.attentionConfig().headDimensionKv(4096) == 128);

    CHECK(config.normConfig().rmsNormEps() == 1e-5f);
    CHECK(config.ropeConfig().ropeDimensionCount() == 128);
    CHECK(config.ropeConfig().ropeFreqBase() == 500000.0f);
    CHECK(config.feedForwardConfig().feedForwardLength() == 14336);

    const auto embedding = config.transformerConfig().tokenEmbeddingShape();
    CHECK(embedding.vocabulary == 128256);
    CHECK(embedding.dimension == 4096);

    const auto q = config.attentionConfig().qProjectionShape(4096);
    const auto k = config.attentionConfig().kProjectionShape(4096);

    CHECK(q.outputDimension == 4096);
    CHECK(q.inputDimension == 4096);
    CHECK(q.isSquare());

    CHECK(k.outputDimension == 1024);
    CHECK(k.inputDimension == 4096);
}

TEST_CASE("Qwen3Instruct2507Config provides the Qwen3 preset", "[model][config][qwen3][usage]")
{
    arch::qwen::Qwen3Instruct2507Config config;

    REQUIRE(config.isValid());

    CHECK(config.archConfig().arch() == ModelArchitecture::Qwen3);
    CHECK(config.architectureName() == "qwen3");
    CHECK(config.archConfig().modelName() == "Qwen3-4B-Instruct-2507");
    CHECK(config.archConfig().hiddenActivation() == "silu");

    CHECK(config.transformerConfig().contextLength() == 262144);
    CHECK(config.transformerConfig().embeddingLength() == 2560);
    CHECK(config.transformerConfig().blockCount() == 36);
    CHECK(config.transformerConfig().vocabSize() == 151936);

    CHECK(config.attentionConfig().headCount() == 32);
    CHECK(config.attentionConfig().headCountKv() == 8);
    CHECK(config.attentionConfig().headDimension(2560) == 128);
    CHECK(config.attentionConfig().headDimensionKv(2560) == 128);

    CHECK(config.normConfig().rmsNormEps() == 1e-6f);
    CHECK(config.ropeConfig().ropeDimensionCount() == 128);
    CHECK(config.ropeConfig().ropeFreqBase() == 5000000.0f);
    CHECK(config.feedForwardConfig().feedForwardLength() == 9728);

    CHECK_FALSE(config.hasMoeConfig());
}

TEST_CASE("ModelConfig represents optional MoE routing explicitly", "[model][config][moe][usage]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    CHECK_FALSE(config.hasMoeConfig());

    MoeConfig moe;
    moe.setExpertCount(128);
    moe.setExpertUsedCount(8);

    config.setMoeConfig(moe);

    REQUIRE(config.hasMoeConfig());

    const MoeConfig &stored = config.moeConfig();
    CHECK(stored.expertCount() == 128);
    CHECK(stored.expertUsedCount() == 8);
    CHECK(config.isValid());

    config.clearMoeConfig();

    CHECK_FALSE(config.hasMoeConfig());
    CHECK(config.isValid());
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("ModelConfig rejects incomplete model configuration", "[model][config][edge]")
{
    ModelConfig config;

    CHECK_FALSE(config.isValid());

    config.archConfig().setArch(ModelArchitecture::Llama);
    config.archConfig().setArchName("llama");
    config.archConfig().setModelName("test");

    CHECK_FALSE(config.isValid());

    config.transformerConfig().setContextLength(8192);
    config.transformerConfig().setEmbeddingLength(4096);
    config.transformerConfig().setBlockCount(32);
    config.transformerConfig().setVocabSize(128256);

    CHECK_FALSE(config.isValid());

    config.attentionConfig().setHeadCount(32);
    config.attentionConfig().setHeadCountKv(8);
    config.attentionConfig().setKeyLength(128);
    config.attentionConfig().setValueLength(128);

    CHECK_FALSE(config.isValid());

    config.normConfig().setRmsNormEps(1e-5f);

    CHECK_FALSE(config.isValid());

    config.ropeConfig().setRopeDimensionCount(128);
    config.ropeConfig().setRopeFreqBase(500000.0f);

    CHECK_FALSE(config.isValid());

    config.feedForwardConfig().setFeedForwardLength(14336);

    CHECK(config.isValid());
}

TEST_CASE("TransformerConfig requires complete transformer dimensions", "[model][config][transformer][edge]")
{
    TransformerConfig config;

    CHECK_FALSE(config.isValid());

    config.setContextLength(8192);
    config.setEmbeddingLength(4096);
    config.setBlockCount(32);
    config.setVocabSize(128256);

    CHECK(config.isValid());
}

TEST_CASE("AttentionConfig requires complete attention geometry", "[model][config][attention][edge]")
{
    AttentionConfig config;

    CHECK_FALSE(config.isValid());

    config.setHeadCount(32);
    CHECK_FALSE(config.isValid());

    config.setHeadCountKv(8);

    CHECK(config.isValid());
}

TEST_CASE("FeedForwardConfig requires an intermediate dimension", "[model][config][ffn][edge]")
{
    FeedForwardConfig config;

    CHECK_FALSE(config.isValid());

    config.setFeedForwardLength(14336);

    CHECK(config.isValid());
}

TEST_CASE("ModelConfig rejects invalid transformer configuration", "[model][config][transformer][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(config.isValid());

    config.transformerConfig().setContextLength(0);
    CHECK_FALSE(config.isValid());

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.transformerConfig().setEmbeddingLength(0);
    CHECK_FALSE(config.isValid());

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.transformerConfig().setBlockCount(0);
    CHECK_FALSE(config.isValid());

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.transformerConfig().setVocabSize(0);
    CHECK_FALSE(config.isValid());
}

TEST_CASE("ModelConfig rejects invalid attention configuration", "[model][config][attention][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(config.isValid());

    config.attentionConfig().setHeadCount(0);
    CHECK_FALSE(config.isValid());

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.attentionConfig().setHeadCountKv(0);
    CHECK_FALSE(config.isValid());
}

TEST_CASE("ModelConfig rejects invalid feed-forward configuration", "[model][config][ffn][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(config.isValid());

    config.feedForwardConfig().setFeedForwardLength(0);

    CHECK_FALSE(config.isValid());
}

TEST_CASE("ModelConfig rejects invalid MoE routing when MoE is present", "[model][config][moe][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    auto &moe = config.moeConfig();
    moe.setExpertCount(8);
    moe.setExpertUsedCount(9);

    CHECK(config.hasMoeConfig());
    CHECK_FALSE(config.isValid());

    moe.setExpertUsedCount(2);

    CHECK(config.isValid());
}

TEST_CASE("AttentionConfig derives head dimensions when explicit dimensions are absent", "[model][config][attention][edge]")
{
    AttentionConfig config;

    config.setHeadCount(32);
    config.setHeadCountKv(8);

    CHECK(config.headDimension() == 0);
    CHECK(config.headDimensionKv() == 0);
    CHECK(config.headDimension(4096) == 128);
    CHECK(config.headDimensionKv(4096) == 128);
}

TEST_CASE("AttentionConfig honors explicit head dimensions", "[model][config][attention][edge]")
{
    AttentionConfig config;

    config.setHeadCount(16);
    config.setHeadCountKv(8);
    config.setKeyLength(256);
    config.setValueLength(256);

    CHECK(config.headDimension(3584) == 256);
    CHECK(config.headDimensionKv(3584) == 256);

    const auto q = config.qProjectionShape(3584);
    const auto kv = config.kProjectionShape(3584);

    CHECK(q.outputDimension == 4096);
    CHECK(q.inputDimension == 3584);
    CHECK(kv.outputDimension == 2048);
    CHECK(kv.inputDimension == 3584);
}

TEST_CASE("Config setters reject invalid floating point values", "[model][config][edge]")
{
    NormConfig norm;
    RopeConfig rope;
    AttentionConfig attention;
    OutputHeadConfig output;

    CHECK_THROWS_AS(norm.setRmsNormEps(0.0f), std::invalid_argument);
    CHECK_THROWS_AS(norm.setRmsNormEps(job::core::safeInfinity()), std::invalid_argument);
    CHECK_THROWS_AS(norm.setRmsNormEps(job::core::safeNaN()), std::invalid_argument);

    CHECK_THROWS_AS(rope.setRopeFreqBase(0.0f), std::invalid_argument);
    CHECK_THROWS_AS(rope.setRopeFreqScale(-1.0f), std::invalid_argument);

    CHECK_THROWS_AS(attention.setAttnLogitSoftCapping(-1.0f), std::invalid_argument);
    CHECK_THROWS_AS(output.setFinalLogitSoftCapping(-1.0f), std::invalid_argument);
}

TEST_CASE("SamplerConfig rejects invalid sampling ranges", "[model][config][sampler][edge]")
{
    SamplerConfig config;

    REQUIRE(config.isValid());

    config.setTopP(1.1f);
    CHECK_FALSE(config.isValid());

    config.setTopP(0.9f);
    config.setMinP(-0.1f);
    CHECK_FALSE(config.isValid());

    config.setMinP(0.05f);
    config.setRepeatPenalty(0.9f);
    CHECK_FALSE(config.isValid());
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================
//
// Nothing useful to benchmark here yet. These are small value-semantic
// configuration objects; reader/parser benchmarks belong with their I/O tests.
//