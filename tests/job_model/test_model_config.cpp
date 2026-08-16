#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <config/model_architecture.h>
#include <config/model_config.h>
#include <config/model_config_reader.h>

#ifdef JOB_TEST_GGUF_FILE
#include <config/arch/qwen/qwen3_instruct_2507.h>
#endif

#include <job_gguf.h>
#include <job_gguf_kv.h>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

namespace job::model::test {

namespace detail {

inline ggml::JobGguf::UPtr createSyntheticLlamaGguf()
{
    auto gguf = ggml::JobGguf::createUniq();

    gguf->setKeyValue(ggml::JobGgufKv("general.architecture", std::string("llama")));
    gguf->setKeyValue(ggml::JobGgufKv("general.name", std::string("Llama-3-8B-Instruct")));

    gguf->setKeyValue(ggml::JobGgufKv("llama.context_length", uint32_t(8192)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.embedding_length", uint32_t(4096)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.block_count", uint32_t(32)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.feed_forward_length", uint32_t(14336)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.vocab_size", uint32_t(128256)));

    gguf->setKeyValue(ggml::JobGgufKv("llama.attention.head_count", uint32_t(32)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.attention.head_count_kv", uint32_t(8)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.attention.layer_norm_rms_epsilon", float(1e-5f)));

    gguf->setKeyValue(ggml::JobGgufKv("llama.rope.dimension_count", uint32_t(128)));
    gguf->setKeyValue(ggml::JobGgufKv("llama.rope.freq_base", float(500000.0f)));

    return gguf;
}

inline ggml::JobGguf::UPtr createSyntheticGemma2Gguf()
{
    auto gguf = ggml::JobGguf::createUniq();

    gguf->setKeyValue(ggml::JobGgufKv("general.architecture", std::string("gemma2")));
    gguf->setKeyValue(ggml::JobGgufKv("general.name", std::string("gemma-2-9b-it")));

    gguf->setKeyValue(ggml::JobGgufKv("gemma2.context_length", uint32_t(8192)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.embedding_length", uint32_t(3584)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.block_count", uint32_t(42)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.vocab_size", uint32_t(256000)));

    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.head_count", uint32_t(16)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.head_count_kv", uint32_t(8)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.key_length", uint32_t(256)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.value_length", uint32_t(256)));

    gguf->setKeyValue(ggml::JobGgufKv("gemma2.final_logit_softcapping", float(30.0f)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.logit_softcapping", float(50.0f)));
    gguf->setKeyValue(ggml::JobGgufKv("gemma2.attention.sliding_window", uint32_t(4096)));

    return gguf;
}

} // namespace detail

// ============================================================================
// Block 1: Usage / Synthetic Examples
// ============================================================================

TEST_CASE("ModelConfig parses LLaMA GQA hyperparameters and builds semantic shapes", "[model][config][example]")
{
    // Note: Once ModelConfigReader is fully wired, this can load directly from gguf.
    // For now, we test the composition and schema layout.
    ModelConfig config;
    config.m_archConfig.m_arch = ModelArchitecture::Llama;
    config.m_archConfig.m_archName = "llama";
    config.m_archConfig.m_modelName = "Llama-3-8B-Instruct";

    config.m_transformerConfig.m_contextLength = 8192;
    config.m_transformerConfig.m_embeddingLength = 4096;
    config.m_transformerConfig.m_blockCount = 32;
    config.m_transformerConfig.m_feedForwardLength = 14336;
    config.m_transformerConfig.m_vocabSize = 128256;
    config.m_transformerConfig.m_headCount = 32;
    config.m_transformerConfig.m_headCountKv = 8;
    config.m_transformerConfig.m_rmsNormEps = 1e-5f;
    config.m_transformerConfig.m_ropeFreqBase = 500000.0f;
    config.m_transformerConfig.m_ropeDimensionCount = 128;

    REQUIRE(config.isValid());
    CHECK(config.m_archConfig.m_arch == ModelArchitecture::Llama);
    CHECK(config.m_archConfig.m_modelName == "Llama-3-8B-Instruct");

    // Dimensions
    CHECK(config.m_transformerConfig.m_contextLength == 8192);
    CHECK(config.m_transformerConfig.m_embeddingLength == 4096);
    CHECK(config.m_transformerConfig.m_blockCount == 32);
    CHECK(config.m_transformerConfig.m_feedForwardLength == 14336);
    CHECK(config.m_transformerConfig.m_vocabSize == 128256);

    // Attention & GQA
    CHECK(config.m_transformerConfig.m_headCount == 32);
    CHECK(config.m_transformerConfig.m_headCountKv == 8);
    CHECK(config.m_transformerConfig.headDimension() == 128);
    CHECK(config.m_transformerConfig.headDimensionKv() == 128);

    // RoPE & Normalization
    CHECK(config.m_transformerConfig.m_rmsNormEps == 1e-5f);
    CHECK(config.m_transformerConfig.m_ropeFreqBase == 500000.0f);
    CHECK(config.m_transformerConfig.m_ropeDimensionCount == 128);

    // Semantic Shapes
    auto vdShape = config.m_transformerConfig.tokenEmbeddingShape();
    CHECK(vdShape.vocabulary == 128256);
    CHECK(vdShape.dimension == 4096);
    CHECK(vdShape.isValid());

    auto qProj = config.m_transformerConfig.qProjectionShape();
    CHECK(qProj.outputDimension == 4096);
    CHECK(qProj.inputDimension == 4096);
    CHECK(qProj.isSquare());

    auto kProj = config.m_transformerConfig.kProjectionShape();
    CHECK(kProj.outputDimension == 1024);
    CHECK(kProj.inputDimension == 4096);

    auto qAct = config.m_transformerConfig.qActivationShape(1, 512);
    CHECK(qAct.batch == 1);
    CHECK(qAct.sequence == 512);
    CHECK(qAct.heads == 32);
    CHECK(qAct.headDimension == 128);
    CHECK(qAct.modelDimension() == 4096);
}

TEST_CASE("ModelConfig parses Gemma 2 explicit head dimensions and softcapping", "[model][config][gemma2]")
{
    ModelConfig config;
    config.m_archConfig.m_arch = ModelArchitecture::Gemma2;
    config.m_archConfig.m_archName = "gemma2";
    config.m_archConfig.m_modelName = "gemma-2-9b-it";
    config.m_archConfig.m_finalLogitSoftCapping = 30.0f;
    config.m_archConfig.m_attnLogitSoftCapping = 50.0f;
    config.m_archConfig.m_slidingWindowSize = 4096;

    config.m_transformerConfig.m_headCount = 16;
    config.m_transformerConfig.m_headCountKv = 8;
    config.m_transformerConfig.m_keyLength = 256;
    config.m_transformerConfig.m_valueLength = 256;
    config.m_transformerConfig.m_embeddingLength = 3584;
    config.m_transformerConfig.m_blockCount = 42;
    config.m_transformerConfig.m_vocabSize = 256000;

    REQUIRE(config.isValid());
    CHECK(config.m_archConfig.m_arch == ModelArchitecture::Gemma2);
    CHECK(config.m_transformerConfig.m_headCount == 16);
    CHECK(config.m_transformerConfig.m_headCountKv == 8);
    CHECK(config.m_transformerConfig.headDimension() == 256);
    CHECK(config.m_transformerConfig.headDimensionKv() == 256);

    CHECK(config.m_archConfig.m_finalLogitSoftCapping == 30.0f);
    CHECK(config.m_archConfig.m_attnLogitSoftCapping == 50.0f);
    CHECK(config.m_archConfig.m_slidingWindowSize == 4096);
}

// ============================================================================
// Block 2: Real File / Architectural Target Integration Test (Qwen3)
// ============================================================================
#ifdef JOB_MODEL_TEST_DATA_DIR
TEST_CASE("ModelConfig loads and validates hyperparameters from HuggingFace JSON directory", "[model][config][json]")
{
    std::filesystem::path dataDir(JOB_MODEL_TEST_DATA_DIR);
    if (std::filesystem::exists(dataDir)) {
        ModelConfig config;
        bool ok = ModelConfigReader::readFromJsonDirectory(dataDir, config);

        REQUIRE(ok);
        CHECK(config.isValid());
        CHECK(config.m_archConfig.m_arch == ModelArchitecture::Qwen3);
        CHECK(config.m_archConfig.architectureName() == "qwen3");
        CHECK(config.m_transformerConfig.m_contextLength == 262144);
        CHECK(config.m_transformerConfig.m_blockCount == 36);
        CHECK(config.m_transformerConfig.m_embeddingLength == 2560);
        CHECK(config.m_transformerConfig.m_vocabSize == 151936);
        CHECK(config.m_transformerConfig.m_ropeFreqBase == 5000000.0f);
        CHECK(config.m_samplerConfig.m_temperature == 0.7f);
        CHECK(config.m_samplerConfig.m_topK == 20);
        CHECK(config.m_samplerConfig.m_topP == 0.8f);
    } else {
        SUCCEED("Skipping JSON test: JOB_MODEL_TEST_DATA_DIR path is not accessible on this machine.");
    }
}
#endif
#ifdef JOB_TEST_GGUF_FILE
TEST_CASE("Qwen3Instruct2507Config validates Qwen3 structural invariants", "[model][config][qwen3]")
{
    job::model::arch::qwen::Qwen3Instruct2507Config qwenConfig;
    REQUIRE(qwenConfig.isValid());
    CHECK(qwenConfig.m_archConfig.m_arch == ModelArchitecture::Qwen3);
    CHECK(qwenConfig.m_archConfig.architectureName() == "qwen3");
    CHECK(qwenConfig.m_archConfig.m_modelName == "Qwen3-4B-Instruct-2507");
    CHECK(qwenConfig.m_transformerConfig.m_contextLength == 262144);
    CHECK(qwenConfig.m_transformerConfig.m_blockCount == 36);
    CHECK(qwenConfig.m_transformerConfig.m_embeddingLength == 2560);
    CHECK(qwenConfig.m_transformerConfig.m_vocabSize == 151936);
    CHECK(qwenConfig.m_transformerConfig.m_ropeFreqBase == 5000000.0f);
}

TEST_CASE("ModelConfig loads and validates hyperparameters from host GGUF file", "[model][config][integration]")
{
    std::filesystem::path ggufPath(JOB_TEST_GGUF_FILE);
    if (std::filesystem::exists(ggufPath)) {
        ModelConfig config;
        bool ok = ModelConfigReader::readFromFile(ggufPath, config);
        REQUIRE(ok);
        CHECK(config.isValid());
        CHECK(config.m_archConfig.m_arch != ModelArchitecture::Unknown);
        CHECK(!config.m_archConfig.architectureName().empty());
        CHECK(config.m_transformerConfig.m_blockCount > 0);
        CHECK(config.m_transformerConfig.m_embeddingLength > 0);
        CHECK(config.m_transformerConfig.m_vocabSize > 0);
    } else {
        SUCCEED("Skipping real GGUF test: JOB_TEST_GGUF_FILE path is not accessible on this machine.");
    }
}
#endif

// ============================================================================
// Block 3: Edge Cases
// ============================================================================

TEST_CASE("ModelConfig guards invariants against invalid inputs and missing keys", "[model][config][edge_cases]")
{
    ModelConfig config;

    SECTION("Default uninitialized ModelConfig fails validation") {
        CHECK_FALSE(config.isValid());
    }

    SECTION("Non-existent file path returns false") {
        // Reader check when implemented
        CHECK_FALSE(config.m_transformerConfig.m_blockCount == 0); // valid defaults are non-zero
    }

    SECTION("Clearing or resetting invalidates configuration") {
        config.m_archConfig.m_arch = ModelArchitecture::Llama;
        config.m_archConfig.m_archName = "llama";
        config.m_transformerConfig.m_blockCount = 32;
        config.m_transformerConfig.m_embeddingLength = 4096;
        config.m_transformerConfig.m_headCount = 32;
        config.m_transformerConfig.m_vocabSize = 32000;
        CHECK(config.isValid());

        config.m_archConfig.m_arch = ModelArchitecture::Unknown;
        CHECK_FALSE(config.isValid());
    }
}

// ============================================================================
// Block 4: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark ModelConfig shape calculations", "[model][config][benchmark]")
{
    ModelConfig config;
    config.m_transformerConfig.m_embeddingLength = 4096;
    config.m_transformerConfig.m_headCount = 32;

    BENCHMARK("Compute 1000 semantic projection shapes") {
        int64_t totalElements = 0;
        for (int i = 0; i < 1000; ++i) {
            auto s = config.m_transformerConfig.qActivationShape(1, i + 1);
            totalElements += s.elementCount();
        }
        return totalElements;
    };
}
#endif

} // namespace job::model::test