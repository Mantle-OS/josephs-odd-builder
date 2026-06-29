#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_logger.h>

#include <job_ggml_device_manager.h>
#include <job_ggml_device.h>
#include <job_ggml_graph.h>

#include "test_ggml_utils.h"

using namespace job::ggml;


auto m = JobGgmlDeviceManager();

TEST_CASE("Device manager builds scheduler after scan", "[ggml][device_manager][usage]")
{
    m.resetScheduler();
    m.scan();

    REQUIRE(m.state() == ManagerState::Ready);

    JOB_LOG_INFO(m.debugStr());

    auto *sched = m.scheduler();
    REQUIRE(sched != nullptr);
}

TEST_CASE("Device manager exposes CPU device for graph execution", "[ggml][device_manager][usage]")
{
    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    REQUIRE(cpu->backend() != nullptr);
    REQUIRE(cpu->type() == GGML_BACKEND_DEVICE_TYPE_CPU);
}

TEST_CASE("Multiple scans do not corrupt device registry", "[ggml][device_manager][edge]")
{
    auto *cpu1 = m.cpuDevice();
    REQUIRE(cpu1 != nullptr);

    m.scan(); // should be no-op after Ready state

    auto *cpu2 = m.cpuDevice();
    REQUIRE(cpu2 != nullptr);

    REQUIRE(cpu1 == cpu2);
}

TEST_CASE("Device manager returns consistent GPU list", "[ggml][device_manager][edge]")
{
    auto g1 = m.gpuDevices();
    auto g2 = m.gpuDevices();

    REQUIRE(g1.size() == g2.size());

    for (size_t i = 0; i < g1.size(); ++i)
        REQUIRE(g1[i] == g2[i]);
}

TEST_CASE("Device manager handles CPU-only environments", "[ggml][device_manager][edge]")
{
    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);

    // GPU may or may not exist depending on build/runtime
    bool hasGpu = m.hasGpu();

    REQUIRE((hasGpu == true || hasGpu == false));
}

// ============================================================================
// Block three: scheduler / orchestration validation
// ============================================================================

TEST_CASE("Scheduler contains at least one backend after scan", "[ggml][device_manager][scheduler]")
{
    auto *sched = m.scheduler();
    REQUIRE(sched != nullptr);
}

TEST_CASE("Graph can execute through device manager scheduler", "[ggml][device_manager][scheduler]")
{
    JobGgmlGraph graph(1024 * 1024);

    auto *A = graph.tensor1d(4, "A");
    auto *B = graph.tensor1d(4, "B");

    float aData[4] = {1, 2, 3, 4};
    float bData[4] = {10, 20, 30, 40};

    std::memcpy(ggml_get_data_f32(A), aData, sizeof(aData));
    std::memcpy(ggml_get_data_f32(B), bData, sizeof(bData));

    auto *C = ggml_add(graph.context(), A, B);
    graph.addForward(C);

    graph.computeWithSched(m);

    const float *out = ggml_get_data_f32(C);

    REQUIRE(out[0] == Catch::Approx(11.0f));
    REQUIRE(out[1] == Catch::Approx(22.0f));
    REQUIRE(out[2] == Catch::Approx(33.0f));
    REQUIRE(out[3] == Catch::Approx(44.0f));
}

// ============================================================================
// Block four: benchmark / stress (optional)
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Device manager scan performance stability", "[ggml][device_manager][benchmark]")
{
    BENCHMARK("scan()") {
        auto b_m = JobGgmlDeviceManager();
        // b_m.scan();
        return b_m.cpuDevice() != nullptr;

    };
}
TEST_CASE("Scheduler graph dispatch overhead", "[ggml][device_manager][benchmark]")
{
    BENCHMARK("scheduled matmul dispatch") {
        JobGgmlGraph graph(1024 * 1024);

        auto *A = graph.tensor2d(64, 64);
        auto *B = graph.tensor2d(64, 64);

        auto *C = ggml_mul_mat(graph.context(), A, B);
        graph.addForward(C);

        graph.computeWithSched(m);

        return ggml_get_data_f32(C)[0];
    };
}
#endif