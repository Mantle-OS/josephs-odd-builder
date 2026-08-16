#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include <job_parallel_for.h>
#include <ctx/job_stealing_ctx.h>
#endif




#include "../transient_test_file.h"

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <io/hf_json_model_config_reader.h>

using namespace job::model;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("HfJsonModelConfigReader reads Qwen3 HuggingFace configuration", "[model][io][hf][usage]")
{
    const std::filesystem::path path{JOB_MODEL_TEST_DATA_DIR};

    HfJsonModelConfigReader reader;
    ModelConfig config;

    REQUIRE(reader.read(path, config));
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
    CHECK(config.attentionConfig().keyLength() == 128);
    CHECK(config.attentionConfig().valueLength() == 128);
    CHECK_FALSE(config.attentionConfig().attentionBias());

    CHECK(config.normConfig().rmsNormEps() == 1e-6f);
    CHECK(config.ropeConfig().ropeFreqBase() == 5000000.0f);
    CHECK(config.feedForwardConfig().feedForwardLength() == 9728);
    CHECK(config.outputHeadConfig().tieWordEmbeddings());
    CHECK_FALSE(config.hasMoeConfig());
}

TEST_CASE("HfJsonModelConfigReader reads HuggingFace generation configuration", "[model][io][hf][sampler][usage]")
{
    const std::filesystem::path path{JOB_MODEL_TEST_DATA_DIR};

    HfJsonModelConfigReader reader;
    ModelConfig config;

    REQUIRE(reader.read(path, config));

    CHECK(config.samplerConfig().temperature() == 0.7f);
    CHECK(config.samplerConfig().topK() == 20);
    CHECK(config.samplerConfig().topP() == 0.8f);
    CHECK_FALSE(config.samplerConfig().greedy());
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("HfJsonModelConfigReader rejects a directory without config.json", "[model][io][hf][edge]")
{
    HfJsonModelConfigReader reader;
    ModelConfig config = arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(config.isValid());

    CHECK_FALSE(reader.read(std::filesystem::path{JOB_MODEL_TEST_DATA_DIR} / "missing", config));
    CHECK_FALSE(config.isValid());
}

TEST_CASE("HfJsonModelConfigReader rejects malformed JSON", "[model][io][hf][edge]")
{
    const std::string badJson = R"({"model_type":"qwen3","hidden_size": nope})";
    const std::vector<std::byte> data{
        reinterpret_cast<const std::byte *>(badJson.data()),
        reinterpret_cast<const std::byte *>(badJson.data() + badJson.size())
    };

    TransientTestFile file{"/tmp/job_model_bad_config.json", data};

    // reader test...
}
// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================


#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark HuggingFace ModelConfig loading", "[model][io][hf][benchmark]")
{
    const std::filesystem::path path{JOB_MODEL_TEST_DATA_DIR};
    HfJsonModelConfigReader reader;
    BENCHMARK("Read Qwen3 HF config into ModelConfig") {
        ModelConfig config;
        const bool ok = reader.read(path, config);
        return ok && config.isValid();
    };
}


TEST_CASE("Benchmark HuggingFace ModelConfig 100X loading", "[model][io][hf][benchmark]")
{
    const std::filesystem::path path{JOB_MODEL_TEST_DATA_DIR};
    HfJsonModelConfigReader reader;

    BENCHMARK("Read Qwen3 HF config into ModelConfig 100x") {
        bool ok = true;
        for (int i = 0; i < 100; ++i) {
            ModelConfig config;
            ok = ok && reader.read(path, config) && config.isValid();
        }
        return ok;
    };
}

TEST_CASE("Benchmark HuggingFace ModelConfig 100X threaded loading", "[model][io][hf][benchmark]")
{
    const std::filesystem::path path{JOB_MODEL_TEST_DATA_DIR};
    HfJsonModelConfigReader reader;
    job::threads::JobStealerCtx ctx{8};

    BENCHMARK("Read Qwen3 HF config into ModelConfig 100x threaded")
    {
        std::atomic_bool ok{true};

        job::threads::parallel_for(
            *ctx.pool, std::size_t{0}, std::size_t{100},
            [&](std::size_t) {
                ModelConfig config;
                if (!reader.read(path, config) || !config.isValid())
                    ok.store(false, std::memory_order_relaxed);
            },
            0, 1);

        return ok.load(std::memory_order_relaxed);
    };
}

#endif

