#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ggml-cpu.h>

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <job_ggml_backend_buffer_type.h>
#include <kv/kv_cache.h>

using namespace job::model;
using namespace job::ggml;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("KvCache allocates a usable cache for a model", "[model][kv][usage]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    REQUIRE(config.isValid());
    REQUIRE(bufferType.isValid());

    KvCache cache{config, bufferType, 1024};

    CHECK(cache.currentPosition() == 0);
    CHECK(cache.maxContextLength() == 1024);
    CHECK(cache.layerCount() == 36);
    CHECK(cache.headCountKv() == 8);
    CHECK(cache.headDimensionKv() == 128);
    CHECK(cache.type() == JobGgmlType::F16);

    REQUIRE(cache.context() != nullptr);
    REQUIRE(cache.context()->isValid());

    REQUIRE(cache.buffer() != nullptr);
    REQUIRE(cache.buffer()->isValid());
    CHECK(cache.totalSizeBytes() > 0);
}

TEST_CASE("KvCache creates key and value tensors for every layer", "[model][kv][usage]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    KvCache cache{config, bufferType, 128};

    REQUIRE(cache.layerCount() == 36);

    for (uint32_t i = 0; i < cache.layerCount(); ++i) {
        const auto &layer = cache.layer(i);

        CHECK(layer.layerIndex == i);

        REQUIRE(layer.k != nullptr);
        REQUIRE(layer.v != nullptr);

        CHECK(layer.k->isValid());
        CHECK(layer.v->isValid());

        CHECK(layer.k->name() == "cache_k_l" + std::to_string(i));
        CHECK(layer.v->name() == "cache_v_l" + std::to_string(i));
    }
}

TEST_CASE("KvCache tracks sequence position", "[model][kv][usage]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    KvCache cache{config, bufferType, 128};

    CHECK(cache.currentPosition() == 0);

    cache.advance();
    CHECK(cache.currentPosition() == 1);

    cache.advance(15);
    CHECK(cache.currentPosition() == 16);

    cache.setPosition(64);
    CHECK(cache.currentPosition() == 64);

    cache.resetPosition();
    CHECK(cache.currentPosition() == 0);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("KvCache uses the model context length when no override is given", "[model][kv][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    config.transformerConfig().setContextLength(128);

    KvCache cache{config, bufferType};

    CHECK(cache.maxContextLength() == 128);
}

TEST_CASE("KvCache derives KV head dimension when value length is absent", "[model][kv][edge]")
{
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    config.attentionConfig().setValueLength(0);

    REQUIRE(config.isValid());

    KvCache cache{config, bufferType, 128};

    CHECK(cache.headDimensionKv() == 128);
}

TEST_CASE("KvCache supports an explicit KV storage type", "[model][kv][edge]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    KvCache cache{config, bufferType, 128, JobGgmlType::F32};

    CHECK(cache.type() == JobGgmlType::F32);
    CHECK(cache.totalSizeBytes() > 0);
}

TEST_CASE("KvCache rejects missing required model geometry", "[model][kv][edge]")
{
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    config.transformerConfig().setBlockCount(0);
    CHECK_THROWS_AS(KvCache(config, bufferType, 128), std::invalid_argument);

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.attentionConfig().setHeadCountKv(0);
    CHECK_THROWS_AS(KvCache(config, bufferType, 128), std::invalid_argument);

    config = arch::qwen::Qwen3Instruct2507Config{};
    config.transformerConfig().setEmbeddingLength(0);
    config.attentionConfig().setKeyLength(0);
    config.attentionConfig().setValueLength(0);
    CHECK_THROWS_AS(KvCache(config, bufferType, 128), std::invalid_argument);
}

TEST_CASE("KvCache layer lookup is bounds checked", "[model][kv][edge]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    KvCache cache{config, bufferType, 128};

    CHECK_NOTHROW(cache.layer(0));
    CHECK_NOTHROW(cache.layer(cache.layerCount() - 1));
    CHECK_THROWS_AS(cache.layer(cache.layerCount()), std::out_of_range);
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark KV cache allocation", "[model][kv][benchmark]")
{
    const ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};
    JobGgmlBackendBufferType bufferType{ggml_backend_cpu_buffer_type()};

    BENCHMARK("Allocate Qwen3 KV cache for 1024 tokens")
    {
        KvCache cache{config, bufferType, 1024};
        return cache.totalSizeBytes();
    };
}

#endif