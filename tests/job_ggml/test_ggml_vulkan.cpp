#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <string>
#include <vector>

#include <job_ggml.h>
#include <job_ggml_context.h>
#include <job_ggml_device_manager.h>
#include <job_ggml_tensor_op.h>
#include <job_ggml_tensor_op_graph.h>
#include <job_ggml_type_traits.h>
#include <job_ggml_vulkan.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Vulkan device manager exposes concrete Vulkan devices",
          "[ggml][device][vulkan][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    REQUIRE(manager->isReady());
    REQUIRE(manager->hasGpu());
    REQUIRE(manager->hasVulkan());

    const JobGgmlDeviceManager::VulkanDevices devices = manager->vulkanDevices();
    REQUIRE_FALSE(devices.isEmpty());

    for (std::size_t index = 0; index < devices.size(); ++index) {
        JobGgmlVulkan *device = manager->vulkan(index);
        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());
        REQUIRE(device->impl() == JobGgmlDeviceImpl::Vulkan);

        JobGgmlDevice *canonical = manager->device(device->uid());
        REQUIRE(canonical != nullptr);
        REQUIRE(canonical == device);
    }
}

TEST_CASE("Vulkan device exposes a Vulkan backend",
          "[ggml][device][vulkan][backend][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *device = manager->vulkan(0);
    REQUIRE(device != nullptr);
    REQUIRE(device->isValid());
    REQUIRE(device->isVulkanBackend());

    const JobGgmlBackend::Ptr backend = device->backend();
    REQUIRE(backend != nullptr);
    REQUIRE(backend->isValid());
    REQUIRE(ggml_backend_is_vk(backend->backend()));
}

TEST_CASE("Vulkan device preserves the Vulkan registry association",
          "[ggml][device][vulkan][registry][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *device = manager->vulkan(0);
    REQUIRE(device != nullptr);

    const JobGgmlBackendReg::Ptr registry = device->backendReg();
    REQUIRE(registry != nullptr);
    REQUIRE(registry->isValid());
    REQUIRE(registry->name() == "Vulkan");
}

TEST_CASE("Vulkan device reports GPU properties",
          "[ggml][device][vulkan][props][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *device = manager->vulkan(0);
    REQUIRE(device != nullptr);

    JobGgmlDeviceProps *props = device->props();
    REQUIRE(props != nullptr);

    const auto type = props->deviceType();
    REQUIRE((type == JobGgmlDeviceType::Gpu || type == JobGgmlDeviceType::IGpu));
    REQUIRE(device->bufferType() != nullptr);
    REQUIRE(device->hostBufferType() != nullptr);
}

TEST_CASE("Vulkan device count agrees with discovered Vulkan devices",
          "[ggml][device][vulkan][count][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const int nativeCount = JobGgmlVulkan::deviceCount();
    REQUIRE(nativeCount >= 0);

    if (nativeCount == 0) {
        CHECK_FALSE(manager->hasVulkan());
        CHECK(manager->vulkanDevices().isEmpty());
        return;
    }

    REQUIRE(manager->hasVulkan());
    CHECK(manager->vulkanDevices().size() == static_cast<std::size_t>(nativeCount));
}

TEST_CASE("Vulkan device dump describes the discovered device",
          "[ggml][device][vulkan][dump][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *device = manager->vulkan(0);
    REQUIRE(device != nullptr);

    const std::string dump = device->dump();
    INFO(dump);

    REQUIRE_FALSE(dump.empty());
    CHECK(dump.find("Vulkan") != std::string::npos);
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Vulkan devices are borrowed indexes into the canonical device collection",
          "[ggml][device][vulkan][edge][canonical]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    const JobGgmlDeviceManager::VulkanDevices vulkanDevices = manager->vulkanDevices();
    const JobGgmlDeviceManager::Devices &devices = manager->devices();

    REQUIRE_FALSE(vulkanDevices.isEmpty());
    REQUIRE_FALSE(devices.isEmpty());

    for (std::size_t index = 0; index < vulkanDevices.size(); ++index) {
        JobGgmlVulkan *vulkan = manager->vulkan(index);
        REQUIRE(vulkan != nullptr);

        JobGgmlDevice *canonical = manager->device(vulkan->uid());
        REQUIRE(canonical != nullptr);

        CHECK(canonical == vulkan);
        CHECK(canonical->device() == vulkan->device());
        CHECK(canonical->impl() == JobGgmlDeviceImpl::Vulkan);
    }
}

TEST_CASE("Vulkan lookup by uid and index preserves canonical identity",
          "[ggml][device][vulkan][edge][identity]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    const JobGgmlDeviceManager::VulkanDevices vulkanDevices = manager->vulkanDevices();
    REQUIRE_FALSE(vulkanDevices.isEmpty());

    for (std::size_t index = 0; index < vulkanDevices.size(); ++index) {
        JobGgmlVulkan *byIndex = manager->vulkan(index);
        REQUIRE(byIndex != nullptr);
        REQUIRE_FALSE(byIndex->uid().empty());

        JobGgmlVulkan *byUid = manager->vulkan(byIndex->uid());
        REQUIRE(byUid != nullptr);

        JobGgmlDevice *generic = manager->device(byIndex->uid());
        REQUIRE(generic != nullptr);

        CHECK(byUid == byIndex);
        CHECK(generic == byIndex);
        CHECK(generic->device() == byIndex->device());
        CHECK(generic->impl() == JobGgmlDeviceImpl::Vulkan);
    }
}

TEST_CASE("Vulkan scheduler automatically includes the CPU fallback backend",
          "[ggml][device][vulkan][backend][sched][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *vulkan = manager->vulkan(0);
    REQUIRE(vulkan != nullptr);
    REQUIRE(vulkan->isValid());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    manager->resetScheduler();
    REQUIRE_FALSE(manager->hasScheduler());

    JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(vulkan);
    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());
    REQUIRE(scheduler->backendCount() == 2);

    REQUIRE(manager->hasScheduler());
    REQUIRE(manager->scheduler() == scheduler);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Vulkan device query performance",
          "[ggml][device][vulkan][benchmark]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    JobGgmlVulkan *device = manager->vulkan(0);
    REQUIRE(device != nullptr);

    BENCHMARK("Vulkan backend type query") {
        return device->isVulkanBackend();
    };
}

TEST_CASE("GGML Vulkan mul_mat 1024 cubed",
          "[ggml][device][vulkan][mul_mat][benchmark]")
{
    constexpr std::int64_t M = 1024;
    constexpr std::int64_t K = 1024;
    constexpr std::int64_t N = 1024;

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    const JobGgmlDeviceManager::VulkanDevices vulkanDevices = manager->vulkanDevices();
    REQUIRE_FALSE(vulkanDevices.isEmpty());

    constexpr std::size_t TargetDeviceIndex = 2;

    JobGgmlVulkan *vulkan = TargetDeviceIndex < vulkanDevices.size()
                                ? manager->vulkan(TargetDeviceIndex)
                                : manager->vulkan(vulkanDevices.size() - 1);

    REQUIRE(vulkan != nullptr);
    REQUIRE(vulkan->isValid());
    REQUIRE(vulkan->isVulkanBackend());

    WARN("Selected Vulkan device: " << vulkan->dump());

    JobGgmlBackend::Ptr backend = vulkan->backend();
    REQUIRE(backend != nullptr);
    REQUIRE(backend->isValid());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    manager->resetScheduler();

    JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(vulkan);
    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());
    REQUIRE(scheduler->backendCount() == 2);

    auto context = JobGgmlContext::createUniqMetadata(1024);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto left  = context->newTensor2d(JobGgmlType::F32, K, M);
    auto right = context->newTensor2d(JobGgmlType::F32, K, N);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    auto mulMat = op->mulMat(*right);
    REQUIRE(mulMat != nullptr);
    REQUIRE(mulMat->isValid());

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(mulMat));
    REQUIRE(expression != nullptr);
    REQUIRE(expression->isValid());

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    scheduler->setTensorBackend(*left, *backend);
    scheduler->setTensorBackend(*right, *backend);
    scheduler->setTensorBackend(*expression, *backend);
    scheduler->splitGraph(*graph);
    REQUIRE(scheduler->allocateGraph(*graph));

    std::vector<float> a(static_cast<std::size_t>(M * K), 0.01f);
    std::vector<float> b(static_cast<std::size_t>(K * N), 0.01f);

    backend->setTensorAsync(*left, a.data(), 0, a.size() * sizeof(float));
    backend->setTensorAsync(*right, b.data(), 0, b.size() * sizeof(float));
    backend->synchronize();

    REQUIRE(scheduler->computeGraph(*graph) == JobGgmlStatus::Success);
    backend->synchronize();

    BENCHMARK("GGML Vulkan mul_mat F32 1024^3 (batched x100)") {
        constexpr int Iterations = 100;

        for (int i = 0; i < Iterations; ++i) {
            if (scheduler->computeGraph(*graph) != JobGgmlStatus::Success)
                FAIL("Graph computation failed during benchmark loop");
        }

        backend->synchronize();
    };
}

TEST_CASE("GGML Vulkan mul_mat 1024 cubed FP16 CoopMat",
          "[ggml][device][vulkan][mul_mat][benchmark]")
{
    constexpr std::int64_t M = 1024;
    constexpr std::int64_t K = 1024;
    constexpr std::int64_t N = 1024;
    constexpr int BatchIterations = 16;

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasVulkan())
        SKIP("No Vulkan GGML device is available on this system");

    const JobGgmlDeviceManager::VulkanDevices vulkanDevices = manager->vulkanDevices();
    REQUIRE_FALSE(vulkanDevices.isEmpty());

    constexpr std::size_t TargetDeviceIndex = 2;

    JobGgmlVulkan *vulkan = TargetDeviceIndex < vulkanDevices.size()
                                ? manager->vulkan(TargetDeviceIndex)
                                : manager->vulkan(vulkanDevices.size() - 1);

    REQUIRE(vulkan != nullptr);
    REQUIRE(vulkan->isValid());
    REQUIRE(vulkan->isVulkanBackend());

    WARN("Benchmarking Vulkan device (FP16): " << vulkan->dump());

    JobGgmlBackend::Ptr backend = vulkan->backend();
    REQUIRE(backend != nullptr);
    REQUIRE(backend->isValid());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    manager->resetScheduler();

    JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(vulkan);
    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());
    REQUIRE(scheduler->backendCount() == 2);

    auto context = JobGgmlContext::createUniqMetadata(1024);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    constexpr JobGgmlType PrecisionType = JobGgmlType::F16;

    auto left  = context->newTensor2d(PrecisionType, K, M);
    auto right = context->newTensor2d(PrecisionType, K, N);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    auto mulMat = op->mulMat(*right);
    REQUIRE(mulMat != nullptr);
    REQUIRE(mulMat->isValid());

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(mulMat));
    REQUIRE(expression != nullptr);
    REQUIRE(expression->isValid());

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    scheduler->setTensorBackend(*left, *backend);
    scheduler->setTensorBackend(*right, *backend);
    scheduler->setTensorBackend(*expression, *backend);
    scheduler->splitGraph(*graph);
    REQUIRE(scheduler->allocateGraph(*graph));

    const std::size_t leftElements  = static_cast<std::size_t>(M * K);
    const std::size_t rightElements = static_cast<std::size_t>(K * N);

    std::vector<ggml_fp16_t> a(leftElements, JobGgmlTypeTraits::fp32ToFp16(0.01f));
    std::vector<ggml_fp16_t> b(rightElements, JobGgmlTypeTraits::fp32ToFp16(0.01f));

    backend->setTensorAsync(*left, a.data(), 0, a.size() * sizeof(ggml_fp16_t));
    backend->setTensorAsync(*right, b.data(), 0, b.size() * sizeof(ggml_fp16_t));
    backend->synchronize();

    REQUIRE(scheduler->computeGraph(*graph) == JobGgmlStatus::Success);
    backend->synchronize();

    BENCHMARK("GGML Vulkan mul_mat F16 1024^3 (batched x16)") {
        for (int i = 0; i < BatchIterations; ++i) {
            if (scheduler->computeGraph(*graph) != JobGgmlStatus::Success)
                FAIL("Graph execution failed during benchmark iteration");
        }

        backend->synchronize();
    };
}

#endif