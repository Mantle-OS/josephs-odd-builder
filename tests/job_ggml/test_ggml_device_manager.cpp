#include <cstddef>
#include <string>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_logger.h>

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>
#include <job_ggml_device_props.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

namespace {

JobGgmlDeviceManager &readyDeviceManager()
{
    JobGgmlDeviceManager &manager =
        testDeviceManager();

    manager.scan();

    return manager;
}

} // namespace

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE(
    "Device manager discovers a usable CPU device",
    "[ggml][device_manager][usage]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    REQUIRE(
        manager.state() ==
        JobGgmlDeviceManager::ManagerState::Ready
        );

    REQUIRE(manager.isReady());
    REQUIRE(manager.isValid());
    REQUIRE(manager.errorString().empty());

    JobGgmlDevice *cpu =
        manager.cpuDevice();

    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    REQUIRE(cpu->props() != nullptr);

    REQUIRE(
        cpu->props()->type() ==
        GGML_BACKEND_DEVICE_TYPE_CPU
        );

    REQUIRE(
        cpu->props()->deviceType() ==
        JobGgmlDeviceType::Cpu
        );

    REQUIRE_FALSE(
        cpu->props()->name().empty()
        );

    JOB_LOG_INFO(manager.debugString());
}

TEST_CASE(
    "Device manager looks up a discovered device by name",
    "[ggml][device_manager][usage]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    JobGgmlDevice *cpu =
        manager.cpuDevice();

    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->props() != nullptr);

    const std::string name =
        cpu->props()->name();

    REQUIRE_FALSE(name.empty());

    JobGgmlDevice *lookup =
        manager.deviceByName(name);

    REQUIRE(lookup != nullptr);

    /*
     * The manager owns canonical device wrappers. A lookup should return the
     * same wrapper, not construct another object around the native device.
     */
    REQUIRE(lookup == cpu);

    const JobGgmlDevice::Ptr sharedLookup =
        manager.deviceByNameShared(name);

    REQUIRE(sharedLookup != nullptr);
    REQUIRE(sharedLookup.get() == cpu);
}

TEST_CASE(
    "Device manager builds a scheduler from discovered devices",
    "[ggml][device_manager][usage][scheduler]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    manager.resetScheduler();

    REQUIRE_FALSE(manager.hasScheduler());
    REQUIRE(manager.scheduler() == nullptr);

    JobGgmlBackendSched::Ptr scheduler =
        manager.buildScheduler();

    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());

    REQUIRE(manager.hasScheduler());
    REQUIRE(manager.scheduler() == scheduler);

    REQUIRE(scheduler->backendCount() >= 1);
    REQUIRE(scheduler->graphSize() == GGML_DEFAULT_GRAPH_SIZE);

    /*
     * GPUs are placed first and the CPU backend last, but a CPU-only system
     * still produces a valid single-backend scheduler.
     */
    for (int index = 0;
         index < scheduler->backendCount();
         ++index) {
        JobGgmlBackend::Ptr backend =
            scheduler->backend(index);

        REQUIRE(backend != nullptr);
        REQUIRE(backend->isValid());

        JobGgmlBackendBufferType::Ptr bufferType =
            scheduler->bufferType(*backend);

        REQUIRE(bufferType != nullptr);
        REQUIRE(bufferType->isValid());
    }
}

TEST_CASE(
    "Device manager exposes complete device object maps",
    "[ggml][device_manager][usage][objects]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    REQUIRE(manager.deviceCount() >= 1);

    for (std::size_t index = 0;
         index < manager.deviceCount();
         ++index) {
        JobGgmlDevice *device =
            manager.device(index);

        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());

        REQUIRE(device->device() != nullptr);

        REQUIRE(
            device->deviceInterface() != nullptr
            );

        REQUIRE(
            device->deviceInterface()->isValid()
            );

        REQUIRE(device->props() != nullptr);
        REQUIRE(device->caps() != nullptr);

        REQUIRE(device->bufferType() != nullptr);
        REQUIRE(device->bufferType()->isValid());

        REQUIRE(device->hasBackend());
        REQUIRE(device->backend() != nullptr);
        REQUIRE(device->backend()->isValid());

        REQUIRE(
            device->hasHostBufferType() ==
            (device->hostBufferType() != nullptr)
            );
    }
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE(
    "Repeated device scans preserve canonical wrapper identity",
    "[ggml][device_manager][edge][identity]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    JobGgmlDevice *firstCpu =
        manager.cpuDevice();

    REQUIRE(firstCpu != nullptr);

    const std::size_t firstDeviceCount =
        manager.deviceCount();

    const std::size_t firstGpuCount =
        manager.gpuDeviceCount();

    const auto firstGpus =
        manager.gpuDevices();

    manager.scan();

    REQUIRE(
        manager.state() ==
        JobGgmlDeviceManager::ManagerState::Ready
        );

    REQUIRE(
        manager.deviceCount() ==
        firstDeviceCount
        );

    REQUIRE(
        manager.gpuDeviceCount() ==
        firstGpuCount
        );

    REQUIRE(
        manager.cpuDevice() ==
        firstCpu
        );

    const auto secondGpus =
        manager.gpuDevices();

    REQUIRE(
        secondGpus.size() ==
        firstGpus.size()
        );

    for (std::size_t index = 0;
         index < firstGpus.size();
         ++index) {
        REQUIRE(
            secondGpus[index] ==
            firstGpus[index]
            );
    }
}

TEST_CASE(
    "GPU availability matches the discovered GPU list",
    "[ggml][device_manager][edge][gpu]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    const auto gpuDevices =
        manager.gpuDevices();

    REQUIRE(
        manager.hasGpu() ==
        !gpuDevices.empty()
        );

    REQUIRE(
        manager.gpuDeviceCount() ==
        gpuDevices.size()
        );

    for (JobGgmlDevice *device : gpuDevices) {
        REQUIRE(device != nullptr);
        REQUIRE(device->isValid());
        REQUIRE(device->props() != nullptr);

        const enum ggml_backend_dev_type type =
            device->props()->type();

        REQUIRE(
            (type == GGML_BACKEND_DEVICE_TYPE_GPU ||
             type == GGML_BACKEND_DEVICE_TYPE_IGPU)
            );
    }
}

TEST_CASE(
    "Device manager rejects out-of-range indexes and empty lookups",
    "[ggml][device_manager][edge][lookup]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    REQUIRE(
        manager.device(manager.deviceCount()) ==
        nullptr
        );

    REQUIRE(
        manager.deviceShared(manager.deviceCount()) ==
        nullptr
        );

    REQUIRE(
        manager.deviceByName("") ==
        nullptr
        );

    REQUIRE(
        manager.deviceByNameShared("") ==
        nullptr
        );

    REQUIRE(
        manager.deviceById("") ==
        nullptr
        );

    REQUIRE(
        manager.deviceByIdShared("") ==
        nullptr
        );

    REQUIRE(
        manager.deviceByName(
            "JOB test device that cannot exist"
            ) == nullptr
        );
}

TEST_CASE(
    "Device memory reporting remains internally consistent",
    "[ggml][device_manager][edge][memory]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    for (const JobGgmlDevice::Ptr &device :
         manager.devices()) {
        REQUIRE(device != nullptr);
        REQUIRE(device->props() != nullptr);

        const std::size_t freeMemory =
            device->props()->memoryFree();

        const std::size_t totalMemory =
            device->props()->memoryTotal();

        /*
         * Some backends may report 0/0 when memory reporting is unavailable.
         * The universally valid invariant is that free memory cannot exceed
         * total memory.
         */
        REQUIRE(freeMemory <= totalMemory);
    }
}

TEST_CASE(
    "Scheduler construction requires a completed device scan",
    "[ggml][device_manager][edge][scheduler]"
    )
{
    JobGgmlDeviceManager manager;

    REQUIRE(
        manager.state() ==
        JobGgmlDeviceManager::ManagerState::Uninitialized
        );

    REQUIRE_FALSE(manager.isReady());
    REQUIRE_FALSE(manager.hasScheduler());

    REQUIRE_THROWS_AS(
        manager.buildScheduler(),
        std::runtime_error
        );
}

TEST_CASE(
    "Scheduler construction rejects an empty explicit device list",
    "[ggml][device_manager][edge][scheduler]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    const std::vector<JobGgmlDevice *> noDevices;

    REQUIRE_THROWS_AS(
        manager.buildScheduler(noDevices),
        std::invalid_argument
        );
}

TEST_CASE(
    "Scheduler reset preserves discovered devices",
    "[ggml][device_manager][edge][scheduler]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    JobGgmlDevice *cpu =
        manager.cpuDevice();

    REQUIRE(cpu != nullptr);

    JobGgmlBackendSched::Ptr scheduler =
        manager.buildScheduler();

    REQUIRE(scheduler != nullptr);
    REQUIRE(manager.hasScheduler());

    manager.resetScheduler();

    REQUIRE_FALSE(manager.hasScheduler());
    REQUIRE(manager.scheduler() == nullptr);

    /*
     * Resetting only the scheduler must not invalidate the canonical device
     * map.
     */
    REQUIRE(manager.isReady());
    REQUIRE(manager.cpuDevice() == cpu);
    REQUIRE(cpu->isValid());
}

TEST_CASE(
    "Device manager reset clears state and allows a new scan",
    "[ggml][device_manager][edge][reset]"
    )
{
    JobGgmlDeviceManager &manager =
        readyDeviceManager();

    REQUIRE(manager.isReady());
    REQUIRE(manager.deviceCount() >= 1);

    REQUIRE(manager.buildScheduler());

    REQUIRE(manager.hasScheduler());

    manager.reset();

    REQUIRE(
        manager.state() ==
        JobGgmlDeviceManager::ManagerState::Uninitialized
        );

    REQUIRE_FALSE(manager.isReady());
    REQUIRE_FALSE(manager.isValid());

    REQUIRE(manager.deviceCount() == 0);
    REQUIRE(manager.gpuDeviceCount() == 0);

    REQUIRE(manager.cpuDevice() == nullptr);
    REQUIRE(manager.scheduler() == nullptr);

    REQUIRE_FALSE(manager.hasCpu());
    REQUIRE_FALSE(manager.hasGpu());
    REQUIRE_FALSE(manager.hasScheduler());

    REQUIRE(manager.errorString().empty());

    /*
     * Restore the shared manager for any test Catch2 chooses to run after
     * this one.
     */
    manager.scan();

    REQUIRE(manager.isReady());
    REQUIRE(manager.isValid());
    REQUIRE(manager.cpuDevice() != nullptr);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Repeated fresh device discovery throughput", "[ggml][device_manager][benchmark][stress]")
{
    BENCHMARK_ADVANCED("50 fresh manager scans")(Catch::Benchmark::Chronometer meter){
        meter.measure([]{
                constexpr std::size_t scans = 50;
                bool ready = true;
                for (std::size_t i = 0; i < scans; ++i) {
                    JobGgmlDeviceManager manager;
                    manager.scan();
                    ready = ready && manager.isReady() && manager.cpuDevice() != nullptr;
                }
                return ready;
            });
    };
}

TEST_CASE("Fresh device discovery performance", "[ggml][device_manager][benchmark][cold]")
{
    BENCHMARK("construct manager and scan") {
        JobGgmlDeviceManager manager;
        manager.scan();
        return manager.isReady() && manager.cpuDevice() != nullptr;
    };
}

TEST_CASE("Repeated fresh device discovery remains stable", "[ggml][device_manager][stress]")
{
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i < iterations; ++i) {
        JobGgmlDeviceManager manager;
        manager.scan();
        REQUIRE(manager.isReady());
        REQUIRE(manager.cpuDevice() != nullptr);
        REQUIRE(manager.deviceCount() >= 1);
    }
}

TEST_CASE("Ready-state scan performance", "[ggml][device_manager][benchmark]")
{
    JobGgmlDeviceManager manager;
    manager.scan();
    REQUIRE(manager.isReady());
    JobGgmlDevice *cpu = manager.cpuDevice();

    REQUIRE(cpu != nullptr);
    BENCHMARK("idempotent scan") {
        manager.scan();
        return manager.cpuDevice() == cpu;
    };
}

TEST_CASE("Device lookup performance", "[ggml][device_manager][benchmark]")
{
    JobGgmlDeviceManager &manager = readyDeviceManager();
    JobGgmlDevice *cpu = manager.cpuDevice();

    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->props() != nullptr);
    const std::string name = cpu->props()->name();

    REQUIRE_FALSE(name.empty());

    BENCHMARK("deviceByName") {
        return manager.deviceByName(name);
    };
}

TEST_CASE("Scheduler construction performance", "[ggml][device_manager][benchmark]")
{
    JobGgmlDeviceManager &manager = readyDeviceManager();

    BENCHMARK("build scheduler") {
        manager.resetScheduler();
        return manager.buildScheduler();
    };
}

#endif