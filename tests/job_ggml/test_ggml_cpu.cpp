#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include <job_parallel_for.h>
#include <job_stealing_ctx.h>
#endif

#include <string>

#include <job_ggml_abort_callback.h>
#include <job_ggml_cpu.h>
#include <job_ggml_threadpool.h>
#include <job_ggml_threadpool_params.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

// ============================================================================
// Block 1: Usage / Examples
// ============================================================================

TEST_CASE("CPU device exposes a usable GGML CPU backend",
          "[ggml][device][cpu][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());
    REQUIRE(cpu->impl() == JobGgmlDeviceImpl::Cpu);
    REQUIRE(cpu->isCpuBackend());

    auto backend = cpu->backend();
    REQUIRE(backend != nullptr);
    REQUIRE(backend->isValid());

    INFO(cpu->dump());
}

TEST_CASE("CPU device can use a dedicated GGML thread pool",
          "[ggml][device][cpu][threadpool][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    JobGgmlThreadPoolParams params;
    params.setNThreads(2);
    params.setPrio(JobGgmlSchedPriority::Normal);
    params.setPoll(50);
    REQUIRE(params.isValid());

    auto pool = JobGgmlThreadPool::createUniq(params);
    REQUIRE(pool != nullptr);
    REQUIRE(pool->isValid());
    REQUIRE(pool->nThreads() == 2);

    cpu->setThreadPool(pool.get());

    // The backend only borrows the pool. Clear it before pool destruction.
    cpu->setThreadPool(nullptr);
}

TEST_CASE("CPU device can cancel computation through a JOB callback",
          "[ggml][device][cpu][abort][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    bool abortRequested = false;

    auto callback = JobGgmlAbortCallback::createUniq(
        [](void *userData) {
            return *static_cast<bool *>(userData);
        },
        &abortRequested);

    REQUIRE(callback != nullptr);
    REQUIRE(callback->isValid());

    CHECK_FALSE(callback->invoke());

    abortRequested = true;
    CHECK(callback->invoke());

    cpu->setAbortCallback(callback.get());

    // GGML borrows callbackData(), so remove it before callback destruction.
    cpu->setAbortCallback(nullptr);
}

TEST_CASE("CPU device exposes host capability information",
          "[ggml][device][cpu][features][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    const std::string dump = cpu->dump();
    REQUIRE_FALSE(dump.empty());

    INFO(dump);
}

TEST_CASE("CPU device executes a graph with a dedicated thread pool",
          "[ggml][device][cpu][threadpool][usage]")
{
    CpuExecutionFixture fixture;

    JobGgmlThreadPoolParams params;
    params.setNThreads(4);
    REQUIRE(params.isValid());

    auto threadPool = JobGgmlThreadPool::createUniq(params);
    REQUIRE(threadPool != nullptr);
    REQUIRE(threadPool->isValid());
    REQUIRE(threadPool->nThreads() == 4);

    fixture.cpu()->setThreadPool(threadPool.get());
    REQUIRE(fixture.compute() == JobGgmlStatus::Success);
    fixture.cpu()->setThreadPool(nullptr);
}

TEST_CASE("CPU abort callback stops graph computation",
          "[ggml][device][cpu][abort][usage]")
{
    CpuExecutionFixture fixture;

    std::size_t callbackCount = 0;

    auto callback = JobGgmlAbortCallback::createUniq([&callbackCount](void *) {
        ++callbackCount;
        return true;
    });

    REQUIRE(callback != nullptr);
    REQUIRE(callback->isValid());

    fixture.cpu()->setAbortCallback(callback.get());

    const JobGgmlStatus status = fixture.compute();
    CHECK(callbackCount > 0);
    REQUIRE(status == JobGgmlStatus::Aborted);

    fixture.cpu()->setAbortCallback(nullptr);
}

// ============================================================================
// Block 2: Edge Cases / Invariants
// ============================================================================

TEST_CASE("CPU device manager preserves canonical CPU identity",
          "[ggml][device][cpu][edge][identity]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    JobGgmlDevice *canonical = manager->device(cpu->uid());
    REQUIRE(canonical != nullptr);

    CHECK(canonical == cpu);
    CHECK(canonical->device() == cpu->device());
    CHECK(canonical->impl() == JobGgmlDeviceImpl::Cpu);
}

TEST_CASE("CPU device accepts clearing borrowed runtime helpers",
          "[ggml][device][cpu][edge][borrowed]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    REQUIRE_NOTHROW(cpu->setThreadPool(nullptr));
    REQUIRE_NOTHROW(cpu->setAbortCallback(nullptr));
}

TEST_CASE("CPU thread pool parameters reject invalid configuration",
          "[ggml][device][cpu][edge][threadpool]")
{
    JobGgmlThreadPoolParams params;
    REQUIRE(params.isValid());

    params.setNThreads(0);
    CHECK_FALSE(params.isValid());

    params.reset();
    REQUIRE(params.isValid());

    params.setPoll(101);
    CHECK_FALSE(params.isValid());
}

// just for now to validate the class; after validation this can be removed.
// Temporary dogfooding checks for the current development machine.
#define JOB_JOSEPH_CHECK 1

#ifdef JOB_JOSEPH_CHECK

TEST_CASE("JobGgmlCpu reports the expected development CPU capabilities",
          "[ggml][device][cpu][joseph]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());
    REQUIRE(cpu->impl() == JobGgmlDeviceImpl::Cpu);

    // Intel Core i9-14900K capabilities observed directly from /proc/cpuinfo.
    CHECK(cpu->hasSse3());
    CHECK(cpu->hasSsse3());
    CHECK(cpu->hasAvx());
    CHECK(cpu->hasAvxVnni());
    CHECK(cpu->hasAvx2());
    CHECK(cpu->hasBmi2());
    CHECK(cpu->hasF16c());
    CHECK(cpu->hasFma());

    // These were not reported by /proc/cpuinfo on this machine.
    CHECK_FALSE(cpu->hasAvx512());
    CHECK_FALSE(cpu->hasAvx512Vbmi());
    CHECK_FALSE(cpu->hasAvx512Vnni());
    CHECK_FALSE(cpu->hasAvx512Bf16());
    CHECK_FALSE(cpu->hasAmxInt8());

    // This is an x86-64 host, so the architecture-specific implementations
    // for the other CPU families should not be selected.
    CHECK_FALSE(cpu->hasNeon());
    CHECK_FALSE(cpu->hasArmFma());
    CHECK_FALSE(cpu->hasFp16Va());
    CHECK_FALSE(cpu->hasDotProd());
    CHECK_FALSE(cpu->hasMatMulInt8());
    CHECK_FALSE(cpu->hasSve());
    CHECK(cpu->sveCount() == 0);
    CHECK_FALSE(cpu->hasSme());

    CHECK_FALSE(cpu->hasRiscvV());
    CHECK(cpu->rvvVectorLength() == 0);
    CHECK_FALSE(cpu->hasVsx());
    CHECK_FALSE(cpu->hasVxe());
    CHECK_FALSE(cpu->hasWasmSimd());
}

TEST_CASE("JobGgmlCpu dump reflects the development CPU",
          "[ggml][device][cpu][joseph][dump]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    const std::string dump = cpu->dump();
    INFO(dump);

    CHECK_FALSE(dump.empty());

    // Check some bogus ones.
    CHECK(dump.find("AVX") == std::string::npos);
    CHECK(dump.find("AVX2") == std::string::npos);
    CHECK(dump.find("AVX-VNNI") == std::string::npos);
    CHECK(dump.find("euler_is_the_drunk_cosuin_of_rk4") == std::string::npos);

    // Get the real ones.
    CHECK(dump.find("avx512=false") != std::string::npos);
    CHECK(dump.find("avx512Vnni=false") != std::string::npos);
    CHECK(dump.find("avx=true") != std::string::npos);
    CHECK(dump.find("avx2=true") != std::string::npos);
    CHECK(dump.find("avxVnni=true") != std::string::npos);
}

#endif // JOB_JOSEPH_CHECK

// ============================================================================
// Block 3: Benchmarks / Stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("CPU graph execution serial versus parallel thread pool",
          "[ggml][device][cpu][threadpool][benchmark]")
{
    constexpr std::int64_t ElementCount = 1024 * 1024 * 16;

    CpuExecutionFixture serialFixture{ElementCount};

    JobGgmlThreadPoolParams singlePoolParams;
    singlePoolParams.setNThreads(1);
    REQUIRE(singlePoolParams.isValid());

    auto singlePool = JobGgmlThreadPool::createUniq(singlePoolParams);
    REQUIRE(singlePool != nullptr);
    REQUIRE(singlePool->isValid());
    REQUIRE(singlePool->nThreads() == 1);

    serialFixture.cpu()->setThreadPool(singlePool.get());

    CpuExecutionFixture threadedFixture{ElementCount};

    JobGgmlThreadPoolParams parallelParams;
    const int parallelThreads = JobGgmlThreadPoolParams::recommendedThreadCount();
    parallelParams.setNThreads(parallelThreads);
    REQUIRE(parallelParams.isValid());

    auto parallelPool = JobGgmlThreadPool::createUniq(parallelParams);
    REQUIRE(parallelPool != nullptr);
    REQUIRE(parallelPool->isValid());
    REQUIRE(parallelPool->nThreads() == parallelThreads);

    threadedFixture.cpu()->setThreadPool(parallelPool.get());

    BENCHMARK("Serial CPU graph execution") {
        return serialFixture.compute();
    };

    BENCHMARK("CPU graph execution - parallel") {
        return threadedFixture.compute();
    };

    serialFixture.cpu()->setThreadPool(nullptr);
    threadedFixture.cpu()->setThreadPool(nullptr);
}

TEST_CASE("CPU serial CpuExecutionFixture 8x over using JOB thread pool",
          "[ggml][device][cpu][threadpool][benchmark][job]")
{
    static constexpr std::size_t GraphCount = 8;
    static constexpr std::int64_t ElementCount = 1024 * 1024 * 16;

    // There is no need for a GGML pool here. JOB supplies the outer parallelism.
    CpuExecutionFixture serialFixture{ElementCount};

    job::threads::JobStealerCtx stealer{12};
    REQUIRE(stealer.pool != nullptr);
    REQUIRE(stealer.pool->workerCount() > 1);

    BENCHMARK("8 times the work serial GGML graphs through JOB threads") {
        job::threads::parallel_for(
            *stealer.pool,
            std::size_t{0},
            GraphCount,
            [&](std::size_t i) {
                (void)i;
                (void)serialFixture.compute();
            },
            0,
            1,
            job::threads::AccessPattern::Linear);
    };
}

#endif