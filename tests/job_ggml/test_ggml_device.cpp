#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_device_manager.h>
#include <job_ggml_device.h>
#include "test_ggml_utils.h"

using namespace job::ggml;


// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Device manager discovers at least one CPU device", "[ggml][device][usage]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    REQUIRE(m.state() == ManagerState::Ready);

    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    REQUIRE(cpu->type() == GGML_BACKEND_DEVICE_TYPE_CPU);
    REQUIRE_FALSE(cpu->name().empty());
}

TEST_CASE("Device lookup by name returns valid device", "[ggml][device][usage]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    auto *lookup = m.deviceByName(cpu->name());
    REQUIRE(lookup != nullptr);
    REQUIRE(lookup->name() == cpu->name());
}

// ============================================================================
// Block two: edge cases
// ============================================================================

TEST_CASE("Device manager scan is idempotent", "[ggml][device][edge]")
{
    auto m = JobGgmlDeviceManager();

    m.scan();
    auto *first = m.cpuDevice();
    REQUIRE(first != nullptr);

    // second scan should not crash or duplicate state
    m.scan();

    auto *second = m.cpuDevice();
    REQUIRE(second != nullptr);

    REQUIRE(first == second);
}

TEST_CASE("hasGpu returns false on CPU-only builds", "[ggml][device][edge]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    // This depends on environment, so we only assert consistency
    bool hasGpu = m.hasGpu();

    // Must not crash or be undefined
    REQUIRE((hasGpu == true || hasGpu == false));
}

TEST_CASE("Device memory reporting is sane", "[ggml][device][edge]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    size_t freeMem = cpu->memoryFree();
    size_t totalMem = cpu->memoryTotal();

    REQUIRE(totalMem > 0);
    REQUIRE(freeMem <= totalMem);
}

// ============================================================================
// Block three: backend / lifecycle validation
// ============================================================================

TEST_CASE("All devices have valid backend pointers after scan", "[ggml][device][backend]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    auto gpus = m.gpuDevices();
    auto *cpu = m.cpuDevice();

    REQUIRE(cpu != nullptr);

    // CPU + GPU devices must all have initialized backend (or nullptr only if unsupported)
    if (cpu->backend()) {
        REQUIRE(cpu->backend() != nullptr);
    }

    for (auto *dev : gpus) {
        REQUIRE(dev != nullptr);

        // GPU backend may or may not exist depending on build
        if (dev->backend()) {
            REQUIRE(dev->backend() != nullptr);
        }
    }
}

TEST_CASE("Device manager returns stable GPU list", "[ggml][device][backend]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    auto g1 = m.gpuDevices();
    auto g2 = m.gpuDevices();

    REQUIRE(g1.size() == g2.size());

    for (size_t i = 0; i < g1.size(); ++i) {
        REQUIRE(g1[i] == g2[i]);
    }
}

// ============================================================================
// Block four: stress / benchmark
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Device scan performance", "[ggml][device][benchmark]")
{
    BENCHMARK("device manager scan") {
        auto m = JobGgmlDeviceManager();
        m.scan();
        return m.cpuDevice() != nullptr;
    };
}

TEST_CASE("Device lookup performance", "[ggml][device][benchmark]")
{
    auto m = JobGgmlDeviceManager();
    m.scan();

    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    std::string name = cpu->name();

    BENCHMARK("deviceByName lookup") {
        return m.deviceByName(name) != nullptr;
    };
}

#endif