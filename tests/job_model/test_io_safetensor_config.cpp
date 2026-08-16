#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <filesystem>

#include <io/safetensors_model_config_reader.h>

using namespace job::model;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("SafeTensorsModelConfigReader rejects an unsupported architecture",
          "[model][io][safetensors][edge][integration]")
{
    const std::filesystem::path path{JOB_TEST_SAFETENSORS_DIR};

    REQUIRE(std::filesystem::is_regular_file(path / "model.safetensors"));
    REQUIRE(std::filesystem::is_regular_file(path / "config.json"));

    SafeTensorsModelConfigReader reader;
    ModelConfig config;

    CHECK_FALSE(reader.read(path, config));
    CHECK(config.archConfig().arch() == ModelArchitecture::Unknown);
    CHECK_FALSE(config.isValid());
}