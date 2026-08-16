#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <job_ggml.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <config/device_config.h>
#include <job_model.h>

using namespace job::model;
using namespace job::ggml;

namespace job::model {

struct JobModelCpuFixture
{
    JobGgmlDeviceManager manager;

    JobGgmlCpu *cpu{nullptr};
    JobGgmlBackendSched::Ptr scheduler;

    JobModelCpuFixture()
    {
        REQUIRE(manager.isReady());
        REQUIRE(manager.isValid());
        REQUIRE(manager.hasCpu());

        cpu = manager.cpu();

        REQUIRE(cpu != nullptr);
        REQUIRE(cpu->isValid());
        REQUIRE(cpu->hasBackend());

        REQUIRE(cpu->bufferType() != nullptr);
        REQUIRE(cpu->bufferType()->isValid());

        scheduler = manager.buildScheduler(cpu);

        REQUIRE(scheduler != nullptr);
        REQUIRE(scheduler->isValid());

        REQUIRE(manager.hasScheduler());
        REQUIRE(manager.scheduler() == scheduler);

        REQUIRE(scheduler->backendCount() >= 1);
        REQUIRE(scheduler->graphSize() == GGML_DEFAULT_GRAPH_SIZE);
    }
};

} // namespace job::model

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("JobModel borrows resolved runtime resources", "[model][usage]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    CHECK(&model.device() == fixture.cpu);
    CHECK(&model.scheduler() == fixture.scheduler.get());

    CHECK(model.device().isValid());
    CHECK(model.scheduler().isValid());

    CHECK(model.deviceConfig().isValid());
}

TEST_CASE("JobModel starts unloaded", "[model][usage]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    CHECK_FALSE(model.isLoaded());

    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel shared and unique factories preserve runtime resources", "[model][usage]")
{
    JobModelCpuFixture fixture;

    auto shared = JobModel::createShared(
        *fixture.cpu,
        *fixture.scheduler);

    REQUIRE(shared != nullptr);

    CHECK(&shared->device() == fixture.cpu);
    CHECK(&shared->scheduler() == fixture.scheduler.get());
    CHECK_FALSE(shared->isLoaded());

    auto unique = JobModel::createUniq(
        *fixture.cpu,
        *fixture.scheduler);

    REQUIRE(unique != nullptr);

    CHECK(&unique->device() == fixture.cpu);
    CHECK(&unique->scheduler() == fixture.scheduler.get());
    CHECK_FALSE(unique->isLoaded());
}

TEST_CASE("JobModel reset preserves borrowed runtime resources", "[model][usage]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    model.config() =
        arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(model.config().isValid());

    model.reset();

    CHECK_FALSE(model.isLoaded());
    CHECK_FALSE(model.config().isValid());

    CHECK(&model.device() == fixture.cpu);
    CHECK(&model.scheduler() == fixture.scheduler.get());

    CHECK(model.deviceConfig().isValid());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel cannot generate before loading a model", "[model][usage]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    const std::vector<int32_t> prompt{
        1,
        2,
        3,
        4
    };

    const auto output =
        model.generate(
            std::span<const int32_t>{prompt},
            4);

    CHECK(output.empty());
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("JobModel rejects a missing GGUF path", "[model][edge][io]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    const std::filesystem::path path{
        "/tmp/job_model_this_model_absolutely_does_not_exist.gguf"
    };

    REQUIRE_FALSE(std::filesystem::exists(path));

    CHECK_FALSE(model.load(path));

    CHECK_FALSE(model.isLoaded());
    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel rejects an invalid explicit configuration", "[model][edge][config]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    const std::filesystem::path path{
        JOB_TEST_GGUF_FILE
    };

    REQUIRE(std::filesystem::exists(path));

    ModelConfig invalid;

    REQUIRE_FALSE(invalid.isValid());

    CHECK_FALSE(
        model.load(
            path,
            std::move(invalid),
            16));

    CHECK_FALSE(model.isLoaded());

    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel failed load leaves no partial model state", "[model][edge][state]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    model.config() =
        arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(model.config().isValid());

    const std::filesystem::path path{
        "/tmp/job_model_missing_after_existing_state.gguf"
    };

    REQUIRE_FALSE(std::filesystem::exists(path));

    CHECK_FALSE(model.load(path));

    CHECK_FALSE(model.isLoaded());

    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel metadata-only loading refuses to pretend weights are materialized",
          "[model][edge][io][gguf]")
{
    JobModelCpuFixture fixture;

    DeviceConfig deviceConfig;
    deviceConfig.setNoAlloc(true);

    REQUIRE(deviceConfig.isValid());
    REQUIRE(deviceConfig.noAlloc());

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler,
        deviceConfig
    };

    const std::filesystem::path path{
        JOB_TEST_GGUF_FILE
    };

    REQUIRE(std::filesystem::exists(path));

    const ModelConfig config =
        arch::qwen::Qwen3Instruct2507Config{};

    REQUIRE(config.isValid());

    //
    // noAlloc lets GGUF produce tensor metadata but JobModel deliberately
    // refuses to call that a loaded model until backend-directed weight
    // materialization exists.
    //
    CHECK_FALSE(
        model.load(
            path,
            config,
            16));

    CHECK_FALSE(model.isLoaded());

    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);

    //
    // Application/runtime policy survives model reset.
    //
    CHECK(model.deviceConfig().noAlloc());
}

TEST_CASE("JobModel automatic GGUF configuration also cleans up after noAlloc failure",
          "[model][edge][io][gguf]")
{
    JobModelCpuFixture fixture;

    DeviceConfig deviceConfig;
    deviceConfig.setNoAlloc(true);

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler,
        deviceConfig
    };

    const std::filesystem::path path{
        JOB_TEST_GGUF_FILE
    };

    REQUIRE(std::filesystem::exists(path));

    //
    // This path additionally dogfoods GgufModelConfigReader before the
    // intentional noAlloc materialization failure.
    //
    CHECK_FALSE(
        model.load(
            path,
            16));

    CHECK_FALSE(model.isLoaded());

    CHECK_FALSE(model.config().isValid());
    CHECK_FALSE(model.weights().isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel reset is idempotent", "[model][edge][state]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    CHECK_NOTHROW(model.reset());
    CHECK_NOTHROW(model.reset());
    CHECK_NOTHROW(model.reset());

    CHECK_FALSE(model.isLoaded());

    CHECK(model.weightContext() == nullptr);
    CHECK(model.computeContext() == nullptr);
    CHECK(model.kvCache() == nullptr);
    CHECK(model.graphBuilder() == nullptr);
}

TEST_CASE("JobModel empty prompt generation fails cleanly while unloaded",
          "[model][edge][generate]")
{
    JobModelCpuFixture fixture;

    JobModel model{
        *fixture.cpu,
        *fixture.scheduler
    };

    const std::vector<int32_t> prompt;

    const auto output =
        model.generate(
            std::span<const int32_t>{prompt},
            8);

    CHECK(output.empty());
}

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================
//
// Intentionally empty.
//
// A meaningful JobModel benchmark requires an actually loaded model.
// JOB_TEST_GGUF_FILE currently points at the real Qwen3-4B fixture, so putting
// load() or generate() in the ordinary benchmark suite would benchmark
// multi-gigabyte model I/O / execution rather than JobModel orchestration.
//
// Once we have a tiny supported GGUF fixture, this is where we benchmark:
//
//     load()
//     prefill
//     single-token decode
//     reset / reload
//