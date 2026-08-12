#include <cstddef>
#include <string>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>
#include <job_ggml_device_props.h>

#include "test_ggml_utils.h"

// ============================================================================
// Block one: usage / examples
// ============================================================================
TEST_CASE("Device manager discovers a usable CPU device", "[ggml][device_manager][usage]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    REQUIRE(manager->state() == DeviceManagerState::Ready);
    REQUIRE(manager->isReady());
    REQUIRE(manager->isValid());
    REQUIRE(manager->errorString().empty());
    REQUIRE(manager->hasCpu());

    JobGgmlCpu *cpu = manager->cpu();

    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());
    REQUIRE(cpu->props() != nullptr);
    REQUIRE(cpu->props()->deviceType() == JobGgmlDeviceType::Cpu);
    REQUIRE_FALSE(cpu->uid().empty());
    REQUIRE(cpu->uid() != "unknown");

    INFO(manager->debugString());
}

TEST_CASE("Device manager exposes canonical devices by uid", "[ggml][device_manager][usage][objects]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    const auto &devices = manager->devices();

    REQUIRE_FALSE(devices.isEmpty());
    REQUIRE(manager->deviceCount() == devices.size());

    for (const auto &[uid, device] : devices) {
        REQUIRE_FALSE(uid.empty());
        REQUIRE(uid != "unknown");
        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());
        REQUIRE(device->uid() == uid);
        REQUIRE(manager->device(uid) == device.get());
    }
}

TEST_CASE("Device manager exposes complete device object maps", "[ggml][device_manager][usage][objects]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    for (const auto &[uid, device] : manager->devices()) {
        INFO("uid: " << uid);

        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());
        REQUIRE(device->device() != nullptr);
        REQUIRE(device->deviceInterface() != nullptr);
        REQUIRE(device->deviceInterface()->isValid());
        REQUIRE(device->props() != nullptr);
        REQUIRE(device->caps() != nullptr);
        REQUIRE(device->bufferType() != nullptr);
        REQUIRE(device->bufferType()->isValid());
        REQUIRE(device->hasBackend());
        REQUIRE(device->backend() != nullptr);
        REQUIRE(device->backend()->isValid());
        REQUIRE(device->hasHostBufferType() == (device->hostBufferType() != nullptr));
    }
}

TEST_CASE("Device manager resolves canonical devices by uid and index", "[ggml][device_manager][usage][lookup]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *cpu = manager->cpu();

    REQUIRE(cpu != nullptr);

    const std::string uid = cpu->uid();

    REQUIRE_FALSE(uid.empty());
    REQUIRE(uid != "unknown");
    REQUIRE(manager->device(uid) == cpu);

    bool found = false;
    for (std::size_t idx = 0; idx < manager->deviceCount(); ++idx) {
        JobGgmlDevice *device = manager->device(idx);

        REQUIRE(device != nullptr);

        if (device == cpu)
            found = true;
    }

    REQUIRE(found);
}

TEST_CASE("Device manager builds a scheduler from discovered devices", "[ggml][device_manager][usage][scheduler]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *cpu = manager->cpu();

    REQUIRE(cpu != nullptr);

    manager->resetScheduler();

    REQUIRE_FALSE(manager->hasScheduler());
    REQUIRE(manager->scheduler() == nullptr);

    JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(cpu->uid(),
                                                                 GGML_DEFAULT_GRAPH_SIZE,
                                                                 false,
                                                                 true);

    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());
    REQUIRE(manager->hasScheduler());
    REQUIRE(manager->scheduler() == scheduler);
    REQUIRE(scheduler->backendCount() >= 1);
    REQUIRE(scheduler->graphSize() == GGML_DEFAULT_GRAPH_SIZE);

    for (int index = 0; index < scheduler->backendCount(); ++index) {
        JobGgmlBackend::Ptr backend = scheduler->backend(index);

        REQUIRE(backend != nullptr);
        REQUIRE(backend->isValid());

        JobGgmlBackendBufferType::Ptr bufferType = scheduler->bufferType(*backend);

        REQUIRE(bufferType != nullptr);
        REQUIRE(bufferType->isValid());
    }
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================
TEST_CASE("Repeated device scans preserve canonical wrapper identity", "[ggml][device_manager][edge][identity]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *firstCpu = manager->cpu();

    REQUIRE(firstCpu != nullptr);

    const std::size_t firstDeviceCount = manager->deviceCount();
    const std::string cpuUid = firstCpu->uid();

    manager->scan();

    REQUIRE(manager->state() == DeviceManagerState::Ready);
    REQUIRE(manager->deviceCount() == firstDeviceCount);
    REQUIRE(manager->cpu() == firstCpu);
    REQUIRE(manager->device(cpuUid) == firstCpu);
}

TEST_CASE("Canonical device uids are valid and unique", "[ggml][device_manager][edge][identity]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    const auto &devices = manager->devices();

    REQUIRE_FALSE(devices.isEmpty());

    for (const auto &[uid, device] : devices) {
        REQUIRE_FALSE(uid.empty());
        REQUIRE(uid != "unknown");
        REQUIRE(device != nullptr);
        REQUIRE(device->uid() == uid);
        REQUIRE(manager->device(uid) == device.get());
    }
}

TEST_CASE("Device implementation indexes reference canonical objects", "[ggml][device_manager][edge][identity][index]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

#ifdef JOB_GGML_CUDA
    for (const auto &[uid, device] : manager->cudaDevices()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->impl() == JobGgmlDeviceImpl::Cuda);
        REQUIRE(manager->device(uid) == device);
        REQUIRE(manager->cuda(uid) == device);
    }

    REQUIRE(manager->hasCuda() == !manager->cudaDevices().isEmpty());
#endif

#ifdef JOB_GGML_VULKAN
    for (const auto &[uid, device] : manager->vulkanDevices()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->impl() == JobGgmlDeviceImpl::Vulkan);
        REQUIRE(manager->device(uid) == device);
        REQUIRE(manager->vulkan(uid) == device);
    }

    REQUIRE(manager->hasVulkan() == !manager->vulkanDevices().isEmpty());
#endif

#ifdef JOB_GGML_OPENCL
    for (const auto &[uid, device] : manager->openClDevices()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->impl() == JobGgmlDeviceImpl::OpenCl);
        REQUIRE(manager->device(uid) == device);
        REQUIRE(manager->openCl(uid) == device);
    }

    REQUIRE(manager->hasOpenCl() == !manager->openClDevices().isEmpty());
#endif

#ifdef JOB_GGML_BLAS
    for (const auto &[uid, device] : manager->blasDevices()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->impl() == JobGgmlDeviceImpl::Blas);
        REQUIRE(manager->device(uid) == device);
        REQUIRE(manager->blas(uid) == device);
    }

    REQUIRE(manager->hasBlas() == !manager->blasDevices().isEmpty());
#endif
}

TEST_CASE("Fallback GPU index contains only generic GPU devices", "[ggml][device_manager][edge][gpu][fallback]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    for (const auto &[uid, device] : manager->fallbackGpus()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());
        REQUIRE(device->props() != nullptr);
        REQUIRE(manager->device(uid) == device);

        const JobGgmlDeviceType type = device->props()->deviceType();

        REQUIRE((type == JobGgmlDeviceType::Gpu ||
                 type == JobGgmlDeviceType::IGpu));
    }
}

TEST_CASE("Device manager rejects empty and unknown uid lookups", "[ggml][device_manager][edge][lookup]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    REQUIRE(manager->device("") == nullptr);
    REQUIRE(manager->device("unknown") == nullptr);
    REQUIRE(manager->device("JOB test device that cannot exist") == nullptr);
    REQUIRE(manager->device("JOB test device that cannot exist") == nullptr);
}

TEST_CASE("Device memory reporting remains internally consistent", "[ggml][device_manager][edge][memory]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    for (const auto &[uid, device] : manager->devices()) {
        INFO("uid: " << uid);

        REQUIRE(device != nullptr);
        REQUIRE(device->props() != nullptr);

        const std::size_t freeMemory = device->props()->memoryFree();
        const std::size_t totalMemory = device->props()->memoryTotal();

        REQUIRE(freeMemory <= totalMemory);
    }
}

TEST_CASE("Device manager can defer scanning", "[ggml][device_manager][edge][scan]")
{
    JobGgmlDeviceManager manager{false};

    REQUIRE(manager.state() == DeviceManagerState::Uninitialized);
    REQUIRE_FALSE(manager.isReady());
    REQUIRE_FALSE(manager.isValid());
    REQUIRE_FALSE(manager.hasScheduler());
    REQUIRE(manager.deviceCount() == 0);
    REQUIRE(manager.cpu() == nullptr);

    manager.scan();

    REQUIRE(manager.isReady());
    REQUIRE(manager.isValid());
    REQUIRE(manager.cpu() != nullptr);
}

TEST_CASE("Scheduler construction requires a completed device scan", "[ggml][device_manager][edge][scheduler]")
{
    JobGgmlDeviceManager manager{false};

    REQUIRE(manager.state() == DeviceManagerState::Uninitialized);
    REQUIRE_FALSE(manager.isReady());
    REQUIRE_FALSE(manager.hasScheduler());
    REQUIRE_THROWS_AS(manager.buildScheduler("CPU"), std::runtime_error);
}

TEST_CASE("Scheduler reset preserves canonical devices", "[ggml][device_manager][edge][scheduler]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *cpu = manager->cpu();

    REQUIRE(cpu != nullptr);

    const std::string uid = cpu->uid();
    JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(cpu);

    REQUIRE(scheduler != nullptr);
    REQUIRE(manager->hasScheduler());

    manager->resetScheduler();

    REQUIRE_FALSE(manager->hasScheduler());
    REQUIRE(manager->scheduler() == nullptr);
    REQUIRE(manager->isReady());
    REQUIRE(manager->cpu() == cpu);
    REQUIRE(manager->device(uid) == cpu);
    REQUIRE(cpu->isValid());
}

TEST_CASE("Device manager reset clears state and allows a new scan", "[ggml][device_manager][edge][reset]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    REQUIRE(manager->isReady());
    REQUIRE(manager->deviceCount() >= 1);
    REQUIRE(manager->buildScheduler("CPU"));
    REQUIRE(manager->hasScheduler());

    manager->reset();

    REQUIRE(manager->state() == DeviceManagerState::Uninitialized);
    REQUIRE_FALSE(manager->isReady());
    REQUIRE_FALSE(manager->isValid());
    REQUIRE(manager->deviceCount() == 0);
    REQUIRE(manager->cpu() == nullptr);
    REQUIRE(manager->scheduler() == nullptr);
    REQUIRE_FALSE(manager->hasCpu());
    REQUIRE_FALSE(manager->hasGpu());
    REQUIRE_FALSE(manager->hasScheduler());
    REQUIRE(manager->errorString().empty());
    REQUIRE(manager->devices().isEmpty());
    REQUIRE(manager->fallbackGpus().isEmpty());

#ifdef JOB_GGML_CUDA
    REQUIRE(manager->cudaDevices().isEmpty());
#endif

#ifdef JOB_GGML_VULKAN
    REQUIRE(manager->vulkanDevices().isEmpty());
#endif

#ifdef JOB_GGML_OPENCL
    REQUIRE(manager->openClDevices().isEmpty());
#endif

#ifdef JOB_GGML_BLAS
    REQUIRE(manager->blasDevices().isEmpty());
#endif

    // Restore the shared manager for any test Catch2 chooses to run next.
    manager->scan();

    REQUIRE(manager->isReady());
    REQUIRE(manager->isValid());
    REQUIRE(manager->cpu() != nullptr);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================
#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Repeated cold start device discovery throughput", "[ggml][device_manager][benchmark][stress]")
{

    BENCHMARK_ADVANCED("50 fresh manager scans")(Catch::Benchmark::Chronometer meter) {
        meter.measure([] {
            constexpr std::size_t scans = 50;
            bool ready = true;

            for (std::size_t i = 0; i < scans; ++i) {
                JobGgmlDeviceManager manager{};
                ready = ready && manager.isReady() && manager.cpu() != nullptr;
            }

            return ready;
        });
    };
}

TEST_CASE("Cold start device discovery performance", "[ggml][device_manager][benchmark][cold]")
{
    BENCHMARK("construct manager and scan") {
        JobGgmlDeviceManager manager{};
        return manager.isReady() && manager.cpu() != nullptr;
    };
}

TEST_CASE("100 cold start fresh device discovery remains stable", "[ggml][device_manager][stress]")
{
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i < iterations; ++i) {
        JobGgmlDeviceManager manager{};
        REQUIRE(manager.isReady());
        REQUIRE(manager.cpu() != nullptr);
        REQUIRE(manager.deviceCount() >= 1);
    }
}

TEST_CASE("Ready-state scan performance", "[ggml][device_manager][benchmark]")
{
    JobGgmlDeviceManager manager;
    REQUIRE(manager.isReady());
    JobGgmlDevice *cpu = manager.cpu();
    REQUIRE(cpu != nullptr);
    BENCHMARK("idempotent scan") {
        manager.scan();
        return manager.cpu() == cpu;
    };
}

TEST_CASE("Device uid lookup performance", "[ggml][device_manager][benchmark][lookup]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    const std::string uid = cpu->uid();
    REQUIRE_FALSE(uid.empty());
    REQUIRE(uid != "unknown");
    BENCHMARK("deviceByUid") {
        return manager->device(uid);
    };
}

TEST_CASE("Device name lookup performance", "[ggml][device_manager][benchmark][lookup]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    const std::string name = cpu->props()->name();
    REQUIRE_FALSE(name.empty());
    BENCHMARK("deviceByName") {
        return manager->device(name);
    };
}

TEST_CASE("Scheduler construction performance", "[ggml][device_manager][benchmark]")
{
    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);
    BENCHMARK("build scheduler") {
        manager->resetScheduler();
        return manager->buildScheduler(manager->cpu());
    };
}
#endif