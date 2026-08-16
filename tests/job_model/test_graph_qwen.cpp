#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <string>
#include <string_view>

#include <ggml-cpu.h>

#include <config/model_config.h>
#include <graph/arch/qwen_graph_builder.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <kv/kv_cache.h>
#include <weights/model_weights.h>

using namespace job::model;
using namespace job::ggml;

namespace job::model {

struct QwenGraphFixture
{
    static constexpr uint32_t kContextLength   = 16;
    static constexpr uint32_t kEmbeddingLength = 32;
    static constexpr uint32_t kBlockCount      = 2;
    static constexpr uint32_t kVocabSize       = 128;
    static constexpr uint32_t kHeadCount       = 4;
    static constexpr uint32_t kHeadCountKv     = 2;
    static constexpr uint32_t kHeadDimension   = 8;
    static constexpr uint32_t kIntermediate    = 96;

    ModelConfig config{makeConfig()};

    JobGgmlBackendBufferType bufferType{
        ggml_backend_cpu_buffer_type()
    };

    JobGgmlContext::UPtr weightContext{
        JobGgmlContext::createUniqMetadata(256)
    };

    ModelWeights weights;

    KvCache::UPtr kvCache{
        KvCache::createUniq(
            config,
            bufferType,
            kContextLength,
            JobGgmlType::F32)
    };

    explicit QwenGraphFixture(bool explicitOutput = false)
    {
        REQUIRE(config.isValid());

        REQUIRE(bufferType.isValid());

        REQUIRE(weightContext != nullptr);
        REQUIRE(weightContext->isValid());

        REQUIRE(kvCache != nullptr);
        REQUIRE(kvCache->layerCount() == kBlockCount);

        addGlobalWeights(explicitOutput);

        for (uint32_t i = 0; i < kBlockCount; ++i)
            addLayerWeights(i);

        REQUIRE(weights.loadFromContext(*weightContext, config));
        REQUIRE(weights.isLoaded());
    }

    [[nodiscard]] static ModelConfig makeConfig()
    {
        ModelConfig config;

        auto &arch = config.archConfig();
        arch.setArch(ModelArchitecture::Qwen3);
        arch.setArchName("qwen3");
        arch.setModelName("Tiny-Qwen-Test");
        arch.setHiddenActivation("silu");

        auto &transformer = config.transformerConfig();
        transformer.setContextLength(kContextLength);
        transformer.setEmbeddingLength(kEmbeddingLength);
        transformer.setBlockCount(kBlockCount);
        transformer.setVocabSize(kVocabSize);

        auto &attention = config.attentionConfig();
        attention.setHeadCount(kHeadCount);
        attention.setHeadCountKv(kHeadCountKv);
        attention.setKeyLength(kHeadDimension);
        attention.setValueLength(kHeadDimension);
        attention.setAttentionBias(false);
        attention.setAttnLogitSoftCapping(0.0f);
        attention.setSlidingWindowSize(0);

        auto &norm = config.normConfig();
        norm.setRmsNormEps(1e-6f);

        auto &rope = config.ropeConfig();
        rope.setRopeDimensionCount(kHeadDimension);
        rope.setRopeFreqBase(10000.0f);
        rope.setRopeFreqScale(1.0f);

        auto &ffn = config.feedForwardConfig();
        ffn.setFeedForwardLength(kIntermediate);

        auto &output = config.outputHeadConfig();
        output.setFinalLogitSoftCapping(0.0f);
        output.setTieWordEmbeddings(true);

        config.clearMoeConfig();

        return config;
    }

    [[nodiscard]] JobGgmlTensor::UPtr tensor1d(
        std::string_view name,
        int64_t ne0)
    {
        auto tensor = weightContext->newTensor1d(
            JobGgmlType::F32,
            ne0);

        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());

        tensor->setName(std::string{name});
        return tensor;
    }

    [[nodiscard]] JobGgmlTensor::UPtr tensor2d(
        std::string_view name,
        int64_t ne0,
        int64_t ne1)
    {
        auto tensor = weightContext->newTensor2d(
            JobGgmlType::F32,
            ne0,
            ne1);

        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());

        tensor->setName(std::string{name});
        return tensor;
    }

    void addGlobalWeights(bool explicitOutput)
    {
        auto tokenEmbd = tensor2d(
            "token_embd.weight",
            kEmbeddingLength,
            kVocabSize);

        auto outputNorm = tensor1d(
            "output_norm.weight",
            kEmbeddingLength);

        if (explicitOutput) {
            auto output = tensor2d(
                "output.weight",
                kEmbeddingLength,
                kVocabSize);

            (void)output;
        }

        (void)tokenEmbd;
        (void)outputNorm;
    }

    void addLayerWeights(uint32_t layer)
    {
        const std::string prefix =
            "blk." + std::to_string(layer) + ".";

        auto attnNorm = tensor1d(
            prefix + "attn_norm.weight",
            kEmbeddingLength);

        auto attnQ = tensor2d(
            prefix + "attn_q.weight",
            kEmbeddingLength,
            kHeadCount * kHeadDimension);

        auto attnK = tensor2d(
            prefix + "attn_k.weight",
            kEmbeddingLength,
            kHeadCountKv * kHeadDimension);

        auto attnV = tensor2d(
            prefix + "attn_v.weight",
            kEmbeddingLength,
            kHeadCountKv * kHeadDimension);

        auto attnOut = tensor2d(
            prefix + "attn_output.weight",
            kHeadCount * kHeadDimension,
            kEmbeddingLength);

        auto ffnNorm = tensor1d(
            prefix + "ffn_norm.weight",
            kEmbeddingLength);

        auto ffnGate = tensor2d(
            prefix + "ffn_gate.weight",
            kEmbeddingLength,
            kIntermediate);

        auto ffnUp = tensor2d(
            prefix + "ffn_up.weight",
            kEmbeddingLength,
            kIntermediate);

        auto ffnDown = tensor2d(
            prefix + "ffn_down.weight",
            kIntermediate,
            kEmbeddingLength);

        (void)attnNorm;
        (void)attnQ;
        (void)attnK;
        (void)attnV;
        (void)attnOut;
        (void)ffnNorm;
        (void)ffnGate;
        (void)ffnUp;
        (void)ffnDown;
    }

    [[nodiscard]] static JobGgmlContext::UPtr computeContext()
    {
        return JobGgmlContext::createUniqMetadata(8192);
    }

    [[nodiscard]] static JobGgmlTensor::UPtr tokens(
        JobGgmlContext &ctx,
        uint32_t count,
        JobGgmlType type = JobGgmlType::I32)
    {
        auto tensor = ctx.newTensor1d(type, count);

        REQUIRE(tensor != nullptr);
        REQUIRE(tensor->isValid());

        return tensor;
    }
};

} // namespace job::model

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("QwenGraphBuilder builds a complete forward graph", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();
    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->isValid());

    auto tokens = QwenGraphFixture::tokens(*ctx, 4);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports its default F16 activation path", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 4);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports an existing KV prefix", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 3);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        5,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports tied token embeddings", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture;

    REQUIRE(fixture.weights.hasTiedEmbedding());
    REQUIRE(fixture.weights.output() == fixture.weights.tokenEmbd());

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports a dedicated LM head", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture{true};

    REQUIRE_FALSE(fixture.weights.hasTiedEmbedding());
    REQUIRE(fixture.weights.output() != nullptr);
    REQUIRE(fixture.weights.output() != fixture.weights.tokenEmbd());

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports output logit soft capping", "[model][graph][qwen][usage]")
{
    QwenGraphFixture fixture;

    fixture.config.outputHeadConfig().setFinalLogitSoftCapping(30.0f);

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder supports SiLU GELU and ReLU hidden activations", "[model][graph][qwen][usage]")
{
    for (const std::string_view activation : {"silu", "gelu", "relu"}) {
        QwenGraphFixture fixture;

        fixture.config.archConfig().setHiddenActivation(
            std::string{activation});

        auto ctx = QwenGraphFixture::computeContext();
        auto tokens = QwenGraphFixture::tokens(*ctx, 2);

        QwenGraphBuilder builder{
            fixture.config,
            fixture.weights,
            *fixture.kvCache
        };

        auto graph = builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32);

        REQUIRE(graph != nullptr);
    }
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("QwenGraphBuilder rejects an invalid model configuration", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    ModelConfig invalid;

    CHECK_THROWS_AS(
        QwenGraphBuilder(
            invalid,
            fixture.weights,
            *fixture.kvCache),
        std::invalid_argument);
}

TEST_CASE("QwenGraphBuilder rejects unloaded model weights", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    ModelWeights unloaded;

    CHECK_THROWS_AS(
        QwenGraphBuilder(
            fixture.config,
            unloaded,
            *fixture.kvCache),
        std::invalid_argument);
}

TEST_CASE("QwenGraphBuilder requires I32 input tokens", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();

    auto tokens = QwenGraphFixture::tokens(
        *ctx,
        2,
        JobGgmlType::F32);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    CHECK_THROWS_AS(
        builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("QwenGraphBuilder rejects unsupported hidden activation", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    fixture.config.archConfig().setHiddenActivation(
        "baseball_player_science");

    REQUIRE(fixture.config.isValid());

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    CHECK_THROWS_AS(
        builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32),
        std::invalid_argument);
}

TEST_CASE("QwenGraphBuilder rejects a weight layer count mismatch", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    //
    // The builder deliberately borrows ModelConfig. Mutating the borrowed
    // configuration therefore must still be caught before graph composition.
    //
    fixture.config.transformerConfig().setBlockCount(1);

    REQUIRE(fixture.config.isValid());
    REQUIRE(fixture.weights.layerCount() == 2);

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 1);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    CHECK_THROWS_AS(
        builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32),
        std::runtime_error);
}

TEST_CASE("QwenGraphBuilder rejects a KV layer count mismatch", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    ModelConfig oneLayerConfig = QwenGraphFixture::makeConfig();
    oneLayerConfig.transformerConfig().setBlockCount(1);

    REQUIRE(oneLayerConfig.isValid());

    KvCache oneLayerCache{
        oneLayerConfig,
        fixture.bufferType,
        QwenGraphFixture::kContextLength,
        JobGgmlType::F32
    };

    REQUIRE(oneLayerCache.layerCount() == 1);
    REQUIRE(fixture.config.transformerConfig().blockCount() == 2);

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 1);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        oneLayerCache
    };

    CHECK_THROWS_AS(
        builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32),
        std::runtime_error);
}

TEST_CASE("QwenGraphBuilder rejects requests beyond KV context capacity", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    CHECK_THROWS_AS(
        builder.buildForward(
            *ctx,
            *tokens,
            QwenGraphFixture::kContextLength - 1,
            JobGgmlType::F32),
        std::out_of_range);
}

TEST_CASE("QwenGraphBuilder accepts a request ending exactly at KV capacity", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        QwenGraphFixture::kContextLength - 2,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

TEST_CASE("QwenGraphBuilder derives RoPE dimensions from attention geometry", "[model][graph][qwen][edge]")
{
    QwenGraphFixture fixture;

    fixture.config.ropeConfig().setRopeDimensionCount(0);

    REQUIRE(fixture.config.isValid());

    auto ctx = QwenGraphFixture::computeContext();
    auto tokens = QwenGraphFixture::tokens(*ctx, 2);

    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    auto graph = builder.buildForward(
        *ctx,
        *tokens,
        0,
        JobGgmlType::F32);

    REQUIRE(graph != nullptr);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark QwenGraphBuilder construction", "[model][graph][qwen][benchmark]")
{
    QwenGraphFixture fixture;
    QwenGraphBuilder builder{
        fixture.config,
        fixture.weights,
        *fixture.kvCache
    };

    BENCHMARK("Build complete two-layer tiny Qwen forward graph")
    {
        //
        // GGML contexts are monotonic metadata arenas. Every benchmark
        // invocation gets a fresh compute context so Catch does not eventually
        // invite one more 336-byte guest into a completely full room.
        //
        auto ctx = QwenGraphFixture::computeContext();
        auto tokens = QwenGraphFixture::tokens(*ctx, 4);

        auto graph = builder.buildForward(
            *ctx,
            *tokens,
            0,
            JobGgmlType::F32);

        return graph != nullptr;
    };
}

#endif