#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_logger.h>
#include <job_ggml_model.h>
#include <job_ggml_device_manager.h>

using namespace job::ggml;

// ============================================================================
// Block one: Usage / Examples
// ============================================================================

TEST_CASE("Load GGUF model and inspect metadata", "[ggml][model][usage]")
{
    auto &manager = JobGgmlDeviceManager::instance();
    manager.scan();

    auto *cpu = manager.cpuDevice();
    REQUIRE(cpu != nullptr);

    JobGgmlModel model;

    // NOTE: replace with a real small gguf test model path later
    std::string path = "test_models/tiny.gguf";

    REQUIRE(model.loadGGUF(path, *cpu));

    // --- usage expectations ---
    REQUIRE_FALSE(model.name().empty());
    REQUIRE_FALSE(model.architecture().empty());

    JOB_LOG_INFO("Model name: {}", model.name());
    JOB_LOG_INFO("Architecture: {}", model.architecture());

    // We should always have weights after load
    REQUIRE(model.weights().tensorCount() > 0);
}

// ============================================================================
// Block two: Edge cases
// ============================================================================

TEST_CASE("Loading invalid GGUF path fails gracefully", "[ggml][model][edge]")
{
    auto &manager = JobGgmlDeviceManager::instance();
    auto *cpu = manager.cpuDevice();
    REQUIRE(cpu != nullptr);

    JobGgmlModel model;

    std::string invalidPath = "/this/path/does/not/exist.gguf";

    REQUIRE_FALSE(model.loadGGUF(invalidPath, *cpu));
}

TEST_CASE("Model metadata lookup returns empty string for missing keys", "[ggml][model][edge]")
{
    JobGgmlModel model;

    // no load → empty model state
    REQUIRE(model.metadata("nonexistent.key").empty());
}

// ============================================================================
// Block three: Benchmarks (optional)
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("GGUF model load performance", "[ggml][model][benchmark]")
{
    auto &manager = JobGgmlDeviceManager::instance();
    manager.scan();

    auto *cpu = manager.cpuDevice();
    REQUIRE(cpu != nullptr);

    JobGgmlModel model;

    std::string path = "test_models/tiny.gguf";

    REQUIRE(model.loadGGUF(path, *cpu));

    BENCHMARK("GGUF load + parse + weights bind") {
        REQUIRE(model.loadGGUF(path, *cpu));
        return model.weights().tensorCount();
    };
}

#endif