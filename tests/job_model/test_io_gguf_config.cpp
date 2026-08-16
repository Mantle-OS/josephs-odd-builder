#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <atomic>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <ctx/job_stealing_ctx.h>
#include <job_parallel_for.h>
#endif

#include "../transient_test_file.h"

#include <config/model_config.h>
#include <io/gguf_model_config_reader.h>

using namespace job::model;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("GgufModelConfigReader reads Qwen3 GGUF configuration", "[model][io][gguf][usage]")
{
    GgufModelConfigReader reader;
    ModelConfig config;

    REQUIRE(reader.read(JOB_TEST_GGUF_FILE, config));
    REQUIRE(config.isValid());

    CHECK(config.archConfig().arch() == ModelArchitecture::Qwen3);
    CHECK(config.architectureName() == "qwen3");

    CHECK(config.transformerConfig().contextLength() == 262144);
    CHECK(config.transformerConfig().embeddingLength() == 2560);
    CHECK(config.transformerConfig().blockCount() == 36);
    CHECK(config.transformerConfig().vocabSize() == 151936);

    CHECK(config.attentionConfig().headCount() == 32);
    CHECK(config.attentionConfig().headCountKv() == 8);
    CHECK(config.attentionConfig().headDimension(2560) == 128);
    CHECK(config.attentionConfig().headDimensionKv(2560) == 128);

    CHECK(config.normConfig().rmsNormEps() == 1e-6f);
    CHECK(config.ropeConfig().ropeFreqBase() == 5000000.0f);
    CHECK(config.feedForwardConfig().feedForwardLength() == 9728);
    CHECK_FALSE(config.hasMoeConfig());
}

TEST_CASE("JobGguf exposes Qwen3 model metadata", "[gguf][usage][integration][external]")
{
    const std::filesystem::path filePath{JOB_TEST_GGUF_FILE};

    job::ggml::JobGgmlContext::UPtr ggmlContext;
    job::ggml::JobGguf gguf{&ggmlContext};

    gguf.initParams()->setNoAlloc(true);
    gguf.initParams()->setCreateContext(false);

    REQUIRE(gguf.open(filePath));

    for (const auto &key : {
             "general.architecture",
             "qwen3.context_length",
             "qwen3.embedding_length",
             "qwen3.block_count",
             "qwen3.feed_forward_length",
             "qwen3.attention.head_count",
             "qwen3.attention.head_count_kv",
             "qwen3.attention.layer_norm_rms_epsilon",
             "qwen3.rope.freq_base"
         })
    {
        WARN("GGUF key '" << key << "' present = " << gguf.hasKey(key));
    }
}


// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("GgufModelConfigReader rejects a missing GGUF file", "[model][io][gguf][edge]")
{
    GgufModelConfigReader reader;
    ModelConfig config;

    CHECK_FALSE(reader.read("/tmp/job_model_missing.gguf", config));
    CHECK_FALSE(config.isValid());
}

TEST_CASE("GgufModelConfigReader rejects an empty GGUF file", "[model][io][gguf][edge]")
{
    TransientTestFile file{"/tmp/job_model_empty.gguf"};

    GgufModelConfigReader reader;
    ModelConfig config;

    CHECK_FALSE(reader.read(file.path(), config));
    CHECK_FALSE(config.isValid());
}

TEST_CASE("GgufModelConfigReader rejects corrupt GGUF data", "[model][io][gguf][edge]")
{
    TransientTestFile file{"/tmp/job_model_corrupt.gguf", 4096, '\x7f'};

    GgufModelConfigReader reader;
    ModelConfig config;

    CHECK_FALSE(reader.read(file.path(), config));
    CHECK_FALSE(config.isValid());
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark GGUF ModelConfig loading", "[model][io][gguf][benchmark]")
{
    GgufModelConfigReader reader;

    BENCHMARK("Read Qwen3 GGUF config into ModelConfig")
    {
        ModelConfig config;
        return reader.read(JOB_TEST_GGUF_FILE, config) && config.isValid();
    };
}

// TEST_CASE("Benchmark GGUF ModelConfig 100X loading", "[model][io][gguf][benchmark]")
// {
//     GgufModelConfigReader reader;

//     BENCHMARK("Read Qwen3 GGUF config into ModelConfig 100x")
//     {
//         bool ok = true;

//         for (int i = 0; i < 100; ++i) {
//             ModelConfig config;
//             ok = ok && reader.read(JOB_TEST_GGUF_FILE, config) && config.isValid();
//         }

//         return ok;
//     };
// }

// TEST_CASE("Benchmark GGUF ModelConfig 100X threaded loading", "[model][io][gguf][benchmark]")
// {
//     GgufModelConfigReader reader;
//     job::threads::JobStealerCtx ctx{8};

//     BENCHMARK("Read Qwen3 GGUF config into ModelConfig 100x threaded")
//     {
//         std::atomic_bool ok{true};

//         job::threads::parallel_for(
//             *ctx.pool, std::size_t{0}, std::size_t{100},
//             [&](std::size_t) {
//                 ModelConfig config;
//                 if (!reader.read(JOB_TEST_GGUF_FILE, config) || !config.isValid())
//                     ok.store(false, std::memory_order_relaxed);
//             },
//             0, 1);

//         return ok.load(std::memory_order_relaxed);
//     };
// }

#endif