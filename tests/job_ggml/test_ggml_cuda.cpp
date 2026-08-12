#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include <ggml-backend.h>
#include <ggml-cuda.h>

#include <job_ggml_cuda.h>
#include <job_ggml_tensor_allocator.h>

#include <cuda_runtime.h>

#include "test_ggml_utils.h"

using namespace job::ggml;

// ============================================================================
// Multi-GPU CUDA / P2P
// ============================================================================

TEST_CASE("CUDA peer access between discovered GPUs",
          "[ggml][device][cuda][multi_gpu][p2p][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();

    if (cudaDevices.size() < 2) {
        SUCCEED("CUDA P2P test requires at least two CUDA devices");
        return;
    }

    JobGgmlCuda *cuda0 = manager->cuda(0);
    JobGgmlCuda *cuda1 = manager->cuda(1);
    REQUIRE(cuda0 != nullptr);
    REQUIRE(cuda1 != nullptr);
    REQUIRE(cuda0->isValid());
    REQUIRE(cuda1->isValid());

    REQUIRE(cuda0->cudaDeviceIndex() >= 0);
    REQUIRE(cuda1->cudaDeviceIndex() >= 0);
    REQUIRE(cuda0->cudaDeviceIndex() != cuda1->cudaDeviceIndex());

    const bool can01 = cuda0->canAccessPeer(*cuda1);
    const bool can10 = cuda1->canAccessPeer(*cuda0);

    WARN("CUDA P2P capability: "
         << "CUDA" << cuda0->cudaDeviceIndex()
         << " -> CUDA" << cuda1->cudaDeviceIndex()
         << " = " << can01
         << ", CUDA" << cuda1->cudaDeviceIndex()
         << " -> CUDA" << cuda0->cudaDeviceIndex()
         << " = " << can10);

    if (!can01 && !can10) {
        SUCCEED("CUDA driver reports no peer access between the two GPUs");
        return;
    }

    constexpr std::size_t ElementCount = 1024 * 1024;
    constexpr std::size_t BufferSize   = ElementCount * sizeof(float);

    std::vector<float> source(ElementCount);
    std::vector<float> result(ElementCount, 0.0f);

    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<float>(i % 1024);

    CudaDeviceAllocation cuda0Memory{cuda0->cudaDeviceIndex(), BufferSize};
    CudaDeviceAllocation cuda1Memory{cuda1->cudaDeviceIndex(), BufferSize};
    REQUIRE(cuda0Memory.isValid());
    REQUIRE(cuda1Memory.isValid());

    if (can01) {
        WARN("Testing CUDA0 -> CUDA1 peer copy");

        REQUIRE(cuda0->enablePeerAccess(*cuda1));
        REQUIRE(cudaSetDevice(cuda0->cudaDeviceIndex()) == cudaSuccess);
        REQUIRE(cudaMemcpy(cuda0Memory.ptr,
                           source.data(),
                           BufferSize,
                           cudaMemcpyHostToDevice) == cudaSuccess);

        REQUIRE(cuda0->copyToPeer(cuda1Memory.ptr,
                                  *cuda1,
                                  cuda0Memory.ptr,
                                  BufferSize));

        REQUIRE(cudaSetDevice(cuda1->cudaDeviceIndex()) == cudaSuccess);
        REQUIRE(cudaDeviceSynchronize() == cudaSuccess);
        REQUIRE(cudaMemcpy(result.data(),
                           cuda1Memory.ptr,
                           BufferSize,
                           cudaMemcpyDeviceToHost) == cudaSuccess);

        CHECK(result == source);
    }

    if (can10) {
        WARN("Testing CUDA1 -> CUDA0 peer copy");

        std::fill(result.begin(), result.end(), 0.0f);

        REQUIRE(cuda1->enablePeerAccess(*cuda0));
        REQUIRE(cudaSetDevice(cuda1->cudaDeviceIndex()) == cudaSuccess);
        REQUIRE(cudaMemcpy(cuda1Memory.ptr,
                           source.data(),
                           BufferSize,
                           cudaMemcpyHostToDevice) == cudaSuccess);

        REQUIRE(cuda1->copyToPeer(cuda0Memory.ptr,
                                  *cuda0,
                                  cuda1Memory.ptr,
                                  BufferSize));

        REQUIRE(cudaSetDevice(cuda0->cudaDeviceIndex()) == cudaSuccess);
        REQUIRE(cudaDeviceSynchronize() == cudaSuccess);
        REQUIRE(cudaMemcpy(result.data(),
                           cuda0Memory.ptr,
                           BufferSize,
                           cudaMemcpyDeviceToHost) == cudaSuccess);

        CHECK(result == source);
    }
}

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("CUDA device manager exposes concrete CUDA devices",
          "[ggml][device][cuda][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    REQUIRE(manager->isReady());
    REQUIRE(manager->hasGpu());
    REQUIRE(manager->hasCuda());

    const JobGgmlDeviceManager::CudaDevices devices = manager->cudaDevices();
    REQUIRE_FALSE(devices.isEmpty());

    for (std::size_t index = 0; index < devices.size(); ++index) {
        JobGgmlCuda *cuda = manager->cuda(index);
        REQUIRE(cuda != nullptr);
        REQUIRE(cuda->isValid());
        REQUIRE(cuda->impl() == JobGgmlDeviceImpl::Cuda);

        JobGgmlDevice *canonical = manager->device(cuda->uid());
        REQUIRE(canonical != nullptr);
        REQUIRE(canonical == cuda);
    }
}

TEST_CASE("CUDA device exposes a CUDA backend",
          "[ggml][device][cuda][backend][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *device = manager->cuda(0);
    REQUIRE(device != nullptr);
    REQUIRE(device->isValid());
    REQUIRE(device->isCudaBackend());

    const JobGgmlBackend::Ptr backend = device->backend();
    REQUIRE(backend != nullptr);
    REQUIRE(backend->isValid());
    REQUIRE(ggml_backend_is_cuda(backend->backend()));
}

TEST_CASE("CUDA device preserves the CUDA registry association",
          "[ggml][device][cuda][registry][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *device = manager->cuda(0);
    REQUIRE(device != nullptr);

    const JobGgmlBackendReg::Ptr registry = device->backendReg();
    REQUIRE(registry != nullptr);
    REQUIRE(registry->isValid());
    REQUIRE(registry->name() == GGML_CUDA_NAME);
}

TEST_CASE("CUDA device reports GPU properties",
          "[ggml][device][cuda][props][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *device = manager->cuda(0);
    REQUIRE(device != nullptr);

    JobGgmlDeviceProps *props = device->props();
    REQUIRE(props != nullptr);

    const auto type = props->deviceType();
    REQUIRE((type == JobGgmlDeviceType::Gpu || type == JobGgmlDeviceType::IGpu));
    REQUIRE(device->bufferType() != nullptr);
    REQUIRE(device->hostBufferType() != nullptr);
}

TEST_CASE("CUDA device count agrees with discovered CUDA devices",
          "[ggml][device][cuda][count][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const int nativeCount = JobGgmlCuda::deviceCount();
    REQUIRE(nativeCount >= 0);

    if (nativeCount == 0) {
        CHECK_FALSE(manager->hasCuda());
        CHECK(manager->cudaDevices().isEmpty());
        return;
    }

    REQUIRE(manager->hasCuda());
    CHECK(manager->cudaDevices().size() == static_cast<std::size_t>(nativeCount));
}

TEST_CASE("CUDA device dump describes the discovered device",
          "[ggml][device][cuda][dump][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *device = manager->cuda(0);
    REQUIRE(device != nullptr);

    const std::string dump = device->dump();
    INFO(dump);

    REQUIRE_FALSE(dump.empty());
    CHECK(dump.find(GGML_CUDA_NAME) != std::string::npos);
}

TEST_CASE("CUDA split buffer type supports both discovered GPUs",
          "[ggml][device][cuda][multi_gpu][split]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const JobGgmlDeviceManager::CudaDevices devices = manager->cudaDevices();

    if (devices.size() < 2)
        SKIP("Multi-GPU CUDA test requires at least two CUDA devices");

    const std::array<float, 2> tensorSplit{
        0.5f,
        0.5f
    };

    JobGgmlBackendBufferType::Ptr splitBufferType =
        JobGgmlCuda::splitBufferType(0, tensorSplit);

    REQUIRE(splitBufferType != nullptr);
    REQUIRE(splitBufferType->isValid());
    REQUIRE(splitBufferType->bufferType() != nullptr);
}

TEST_CASE("CUDA split buffer allocates a tensor across both CUDA devices",
          "[ggml][device][cuda][multi_gpu][split][allocation]")
{
    constexpr std::int64_t M = 4096;
    constexpr std::int64_t K = 4096;

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();

    if (cudaDevices.size() < 2)
        SKIP("Multi-GPU CUDA test requires at least two CUDA devices");

    const std::array<float, 2> tensorSplit{
        0.5f,
        0.5f
    };

    JobGgmlBackendBufferType::Ptr splitType =
        JobGgmlCuda::splitBufferType(0, tensorSplit);

    REQUIRE(splitType != nullptr);
    REQUIRE(splitType->isValid());

    auto context = JobGgmlContext::createUniqMetadata(1024);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto weights = context->newTensor2d(JobGgmlType::F16, K, M);
    REQUIRE(weights != nullptr);
    REQUIRE(weights->isValid());

    const std::size_t requiredSize = splitType->allocationSize(*weights);
    REQUIRE(requiredSize > 0);

    auto splitBufferUniq = splitType->allocateBuffer(requiredSize);
    REQUIRE(splitBufferUniq != nullptr);
    REQUIRE(splitBufferUniq->isValid());

    JobGgmlBackendBuffer::Ptr splitBuffer{std::move(splitBufferUniq)};

    auto allocator = JobGgmlTensorAllocator::createUniq(splitBuffer);
    REQUIRE(allocator != nullptr);
    REQUIRE(allocator->isValid());

    REQUIRE(allocator->allocate(*weights) == JobGgmlStatus::Success);

    REQUIRE(weights->tensor()->buffer != nullptr);
    CHECK(weights->tensor()->buffer == splitBuffer->buffer());
    CHECK(splitBuffer->bufferType() == splitType->bufferType());
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("CUDA devices are borrowed indexes into the canonical device collection",
          "[ggml][device][cuda][edge][canonical]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();
    const JobGgmlDeviceManager::Devices &devices = manager->devices();

    REQUIRE_FALSE(cudaDevices.isEmpty());
    REQUIRE_FALSE(devices.isEmpty());

    for (std::size_t index = 0; index < cudaDevices.size(); ++index) {
        JobGgmlCuda *cuda = manager->cuda(index);
        REQUIRE(cuda != nullptr);

        JobGgmlDevice *canonical = manager->device(cuda->uid());
        REQUIRE(canonical != nullptr);

        CHECK(canonical == cuda);
        CHECK(canonical->device() == cuda->device());
        CHECK(canonical->impl() == JobGgmlDeviceImpl::Cuda);
    }
}

TEST_CASE("CUDA lookup by uid and index preserves canonical identity",
          "[ggml][device][cuda][edge][identity]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();
    REQUIRE_FALSE(cudaDevices.isEmpty());

    for (std::size_t index = 0; index < cudaDevices.size(); ++index) {
        JobGgmlCuda *byIndex = manager->cuda(index);
        REQUIRE(byIndex != nullptr);
        REQUIRE(byIndex->device() != nullptr);
        REQUIRE(byIndex->props() != nullptr);

        JobGgmlCuda *byUid = manager->cuda(byIndex->uid());
        REQUIRE(byUid != nullptr);

        JobGgmlDevice *canonical = manager->device(byIndex->uid());
        REQUIRE(canonical != nullptr);

        CHECK(byUid == byIndex);
        CHECK(canonical == byIndex);
        CHECK(canonical->device() == byIndex->device());
        CHECK(canonical->impl() == JobGgmlDeviceImpl::Cuda);
    }
}

TEST_CASE("CUDA host buffer registration rejects invalid input",
          "[ggml][device][cuda][edge][host-buffer]")
{
    CHECK_FALSE(JobGgmlCuda::registerHostBuffer(nullptr, 4096));
    CHECK_FALSE(JobGgmlCuda::registerHostBuffer(reinterpret_cast<void *>(0x1), 0));
    CHECK(JobGgmlCuda::splitBufferType(-1, std::span<const float>{}) == nullptr);
    CHECK(JobGgmlCuda::splitBufferType(0, std::span<const float>{}) == nullptr);
}

TEST_CASE("CUDA device index matches native CUDA backend ordinal",
          "[ggml][device][cuda][index][usage]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();
    REQUIRE_FALSE(cudaDevices.isEmpty());

    for (std::size_t deviceIndex = 0; deviceIndex < cudaDevices.size(); ++deviceIndex) {
        JobGgmlCuda *cuda = manager->cuda(deviceIndex);
        REQUIRE(cuda != nullptr);
        REQUIRE(cuda->isValid());

        const int index = cuda->cudaDeviceIndex();

        INFO(cuda->dump());

        REQUIRE(index >= 0);
        REQUIRE(index < JobGgmlCuda::deviceCount());

        CHECK(cuda->props()->name() ==
              std::string{GGML_CUDA_NAME} + std::to_string(index));
    }
}

TEST_CASE("CUDA all-reduce rejects invalid wrapper collections",
          "[ggml][device][cuda][allreduce][edge]")
{
    const std::vector<JobGgmlBackend::Ptr> noBackends;
    const std::vector<JobGgmlTensor::Ptr> noTensors;

    CHECK_FALSE(JobGgmlCuda::allReduceTensor(noBackends, noTensors));

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *cuda = manager->cuda(0);
    REQUIRE(cuda != nullptr);

    std::vector<JobGgmlBackend::Ptr> backends{
        cuda->backend()
    };

    CHECK_FALSE(JobGgmlCuda::allReduceTensor(backends, noTensors));
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("CUDA device query performance",
          "[ggml][device][cuda][benchmark]")
{
    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    JobGgmlCuda *device = manager->cuda(0);
    REQUIRE(device != nullptr);

    BENCHMARK("CUDA backend type query") {
        return device->isCudaBackend();
    };
}

TEST_CASE("GGML CUDA mul_mat saturation stress FP16",
          "[ggml][device][cuda][mul_mat][f16][stress][benchmark]")
{
    const std::vector<std::int64_t> sizes{
        1024,
        2048,
        4096,
        8192,
        12288,
        16384
    };

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    if (!manager->hasCuda())
        SKIP("No CUDA GGML device is available on this system");

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();
    REQUIRE_FALSE(cudaDevices.isEmpty());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);

    for (std::size_t devIdx = 0; devIdx < cudaDevices.size(); ++devIdx) {
        JobGgmlCuda *cuda = manager->cuda(devIdx);
        REQUIRE(cuda != nullptr);

        const JobGgmlBackend::Ptr backend = cuda->backend();
        REQUIRE(backend != nullptr);
        REQUIRE(backend->isValid());

        manager->resetScheduler();

        JobGgmlBackendSched::Ptr scheduler = manager->buildScheduler(cuda);
        REQUIRE(scheduler != nullptr);
        REQUIRE(scheduler->isValid());

        for (const std::int64_t dim : sizes) {
            const std::string sectionName =
                "CUDA" + std::to_string(devIdx) +
                " - Dimensions " + std::to_string(dim) + "^3";

            DYNAMIC_SECTION(sectionName) {
                benchMatMul(
                    cuda,
                    devIdx,
                    dim,
                    dim,
                    dim,
                    16,
                    1024,
                    scheduler,
                    backend);
            }
        }
    }
}

TEST_CASE("GGML CUDA multi GPU pipeline 2-layer mul_mat 4096 cubed FP16",
          "[ggml][device][cuda][multi_gpu][pipeline][f16][benchmark]")
{
    constexpr std::int64_t M = 4096;
    constexpr std::int64_t K = 4096;
    constexpr std::int64_t N = 4096;
    constexpr int BatchIterations = 16;

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();

    if (cudaDevices.size() < 2) {
        SUCCEED("Multi-GPU pipeline test requires at least two CUDA devices");
        return;
    }

    JobGgmlCuda *cuda0 = manager->cuda(0);
    JobGgmlCuda *cuda1 = manager->cuda(1);
    REQUIRE(cuda0 != nullptr);
    REQUIRE(cuda1 != nullptr);
    REQUIRE(cuda0->isValid());
    REQUIRE(cuda1->isValid());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    const JobGgmlBackend::Ptr cuda0Backend = cuda0->backend();
    const JobGgmlBackend::Ptr cuda1Backend = cuda1->backend();
    const JobGgmlBackend::Ptr cpuBackend   = cpu->backend();
    REQUIRE(cuda0Backend != nullptr);
    REQUIRE(cuda1Backend != nullptr);
    REQUIRE(cpuBackend != nullptr);
    REQUIRE(cuda0Backend->isValid());
    REQUIRE(cuda1Backend->isValid());
    REQUIRE(cpuBackend->isValid());

    WARN("Pipeline Layer 0 on: " << cuda0->dump());
    WARN("Pipeline Layer 1 on: " << cuda1->dump());

    auto context = JobGgmlContext::createUniqMetadata(2048);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    constexpr JobGgmlType PrecisionType = JobGgmlType::F16;

    // Layer 0 -> CUDA0
    auto left1  = context->newTensor2d(PrecisionType, K, M);
    auto right1 = context->newTensor2d(PrecisionType, K, N);
    REQUIRE(left1 != nullptr);
    REQUIRE(right1 != nullptr);

    auto op1 = JobGgmlTensorOp::createUniq(left1->tensor(), context.get());
    REQUIRE(op1 != nullptr);

    auto layer1Out = op1->mulMat(*right1);
    REQUIRE(layer1Out != nullptr);

    // Layer 1 -> CUDA1
    auto right2 = context->newTensor2d(PrecisionType, K, N);
    REQUIRE(right2 != nullptr);

    auto op2 = JobGgmlTensorOp::createUniq(layer1Out->tensor(), context.get());
    REQUIRE(op2 != nullptr);

    auto layer2Out = op2->mulMat(*right2);
    REQUIRE(layer2Out != nullptr);

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(layer2Out));
    REQUIRE(expression != nullptr);
    REQUIRE(expression->isValid());

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    /*
     * Explicit multi-backend scheduler.
     *
     * DeviceManager intentionally owns the common one-device + CPU fallback
     * construction path. This graph explicitly spans CUDA0 and CUDA1, so the
     * scheduler is constructed directly.
     */
    JobGgmlBackendSched::Backends backends;
    backends.push_back(cuda0Backend);
    backends.push_back(cuda1Backend);
    backends.push_back(cpuBackend);

    JobGgmlBackendSched::BufferTypes bufferTypes;

    JobGgmlBackendSched::Ptr scheduler = JobGgmlBackendSched::createShared(
        std::move(backends),
        std::move(bufferTypes),
        GGML_DEFAULT_GRAPH_SIZE,
        false,
        true);

    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());
    REQUIRE(scheduler->backendCount() == 3);

    scheduler->setTensorBackend(*left1, *cuda0Backend);
    scheduler->setTensorBackend(*right1, *cuda0Backend);
    scheduler->setTensorBackend(*right2, *cuda1Backend);
    scheduler->setTensorBackend(*expression, *cuda1Backend);

    scheduler->splitGraph(*graph);
    REQUIRE(scheduler->allocateGraph(*graph));

    const std::size_t elemCount = static_cast<std::size_t>(M * K);

    std::vector<ggml_fp16_t> initData(
        elemCount,
        JobGgmlTypeTraits::fp32ToFp16(0.01f));

    cuda0Backend->setTensorAsync(
        *left1,
        initData.data(),
        0,
        initData.size() * sizeof(ggml_fp16_t));

    cuda0Backend->setTensorAsync(
        *right1,
        initData.data(),
        0,
        initData.size() * sizeof(ggml_fp16_t));

    cuda1Backend->setTensorAsync(
        *right2,
        initData.data(),
        0,
        initData.size() * sizeof(ggml_fp16_t));

    cuda0Backend->synchronize();
    cuda1Backend->synchronize();

    REQUIRE(scheduler->computeGraph(*graph) == JobGgmlStatus::Success);
    cuda0Backend->synchronize();
    cuda1Backend->synchronize();

    BENCHMARK("GGML CUDA multi GPU pipeline 2-layer mul_mat F16 4096^3 (batched x16)") {
        for (int i = 0; i < BatchIterations; ++i) {
            if (scheduler->computeGraph(*graph) != JobGgmlStatus::Success)
                FAIL("Pipeline CUDA graph execution failed");
        }

        cuda0Backend->synchronize();
        cuda1Backend->synchronize();
    };
}

TEST_CASE("GGML CUDA dual GPU independent mul_mat 16384 cubed FP16",
          "[ggml][device][cuda][multi_gpu][independent][f16][benchmark]")
{
    constexpr std::int64_t M = 16384;
    constexpr std::int64_t K = 16384;
    constexpr std::int64_t N = 16384;
    constexpr int BatchIterations  = 16;
    constexpr int WarmupIterations = 16;

    REQUIRE(g_jobGgml != nullptr);

    JobGgmlDeviceManager *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    manager->scan();

    const JobGgmlDeviceManager::CudaDevices cudaDevices = manager->cudaDevices();

    if (cudaDevices.size() < 2)
        SKIP("Independent multi-GPU CUDA benchmark requires two CUDA devices");

    JobGgmlCuda *cuda0 = manager->cuda(0);
    JobGgmlCuda *cuda1 = manager->cuda(1);
    REQUIRE(cuda0 != nullptr);
    REQUIRE(cuda1 != nullptr);
    REQUIRE(cuda0->isValid());
    REQUIRE(cuda1->isValid());

    JobGgmlCpu *cpu = manager->cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    const JobGgmlBackend::Ptr cuda0Backend = cuda0->backend();
    const JobGgmlBackend::Ptr cuda1Backend = cuda1->backend();
    REQUIRE(cuda0Backend != nullptr);
    REQUIRE(cuda1Backend != nullptr);
    REQUIRE(cuda0Backend->isValid());
    REQUIRE(cuda1Backend->isValid());

    WARN("Independent CUDA workload 0: " << cuda0->dump());
    WARN("Independent CUDA workload 1: " << cuda1->dump());

    // ------------------------------------------------------------------------
    // CUDA0 graph
    // ------------------------------------------------------------------------

    auto context0 = JobGgmlContext::createUniqMetadata(1024);
    REQUIRE(context0 != nullptr);
    REQUIRE(context0->isValid());

    auto left0  = context0->newTensor2d(JobGgmlType::F16, K, M);
    auto right0 = context0->newTensor2d(JobGgmlType::F16, K, N);
    REQUIRE(left0 != nullptr);
    REQUIRE(right0 != nullptr);
    REQUIRE(left0->isValid());
    REQUIRE(right0->isValid());

    auto op0 = JobGgmlTensorOp::createUniq(left0->tensor(), context0.get());
    REQUIRE(op0 != nullptr);
    REQUIRE(op0->isValid());

    auto mulMat0 = op0->mulMat(*right0);
    REQUIRE(mulMat0 != nullptr);
    REQUIRE(mulMat0->isValid());

    auto expression0 = JobGgmlTensorOpGraph::wrap(std::move(mulMat0));
    REQUIRE(expression0 != nullptr);
    REQUIRE(expression0->isValid());

    auto graph0 = expression0->buildGraph();
    REQUIRE(graph0 != nullptr);
    REQUIRE(graph0->isValid());

    // ------------------------------------------------------------------------
    // CUDA1 graph
    // ------------------------------------------------------------------------

    auto context1 = JobGgmlContext::createUniqMetadata(1024);
    REQUIRE(context1 != nullptr);
    REQUIRE(context1->isValid());

    auto left1  = context1->newTensor2d(JobGgmlType::F16, K, M);
    auto right1 = context1->newTensor2d(JobGgmlType::F16, K, N);
    REQUIRE(left1 != nullptr);
    REQUIRE(right1 != nullptr);
    REQUIRE(left1->isValid());
    REQUIRE(right1->isValid());

    auto op1 = JobGgmlTensorOp::createUniq(left1->tensor(), context1.get());
    REQUIRE(op1 != nullptr);
    REQUIRE(op1->isValid());

    auto mulMat1 = op1->mulMat(*right1);
    REQUIRE(mulMat1 != nullptr);
    REQUIRE(mulMat1->isValid());

    auto expression1 = JobGgmlTensorOpGraph::wrap(std::move(mulMat1));
    REQUIRE(expression1 != nullptr);
    REQUIRE(expression1->isValid());

    auto graph1 = expression1->buildGraph();
    REQUIRE(graph1 != nullptr);
    REQUIRE(graph1->isValid());

    // Each independent workload gets its own accelerator + CPU scheduler.
    manager->resetScheduler();

    JobGgmlBackendSched::Ptr scheduler0 = manager->buildScheduler(cuda0);
    REQUIRE(scheduler0 != nullptr);
    REQUIRE(scheduler0->isValid());

    manager->resetScheduler();

    JobGgmlBackendSched::Ptr scheduler1 = manager->buildScheduler(cuda1);
    REQUIRE(scheduler1 != nullptr);
    REQUIRE(scheduler1->isValid());

    scheduler0->setTensorBackend(*left0, *cuda0Backend);
    scheduler0->setTensorBackend(*right0, *cuda0Backend);
    scheduler0->setTensorBackend(*expression0, *cuda0Backend);
    scheduler0->splitGraph(*graph0);
    REQUIRE(scheduler0->allocateGraph(*graph0));

    scheduler1->setTensorBackend(*left1, *cuda1Backend);
    scheduler1->setTensorBackend(*right1, *cuda1Backend);
    scheduler1->setTensorBackend(*expression1, *cuda1Backend);
    scheduler1->splitGraph(*graph1);
    REQUIRE(scheduler1->allocateGraph(*graph1));

    const std::size_t leftElements  = static_cast<std::size_t>(M * K);
    const std::size_t rightElements = static_cast<std::size_t>(K * N);

    std::vector<ggml_fp16_t> leftData(
        leftElements,
        JobGgmlTypeTraits::fp32ToFp16(0.01f));

    std::vector<ggml_fp16_t> rightData(
        rightElements,
        JobGgmlTypeTraits::fp32ToFp16(0.01f));

    cuda0Backend->setTensorAsync(
        *left0,
        leftData.data(),
        0,
        leftData.size() * sizeof(ggml_fp16_t));

    cuda0Backend->setTensorAsync(
        *right0,
        rightData.data(),
        0,
        rightData.size() * sizeof(ggml_fp16_t));

    cuda1Backend->setTensorAsync(
        *left1,
        leftData.data(),
        0,
        leftData.size() * sizeof(ggml_fp16_t));

    cuda1Backend->setTensorAsync(
        *right1,
        rightData.data(),
        0,
        rightData.size() * sizeof(ggml_fp16_t));

    cuda0Backend->synchronize();
    cuda1Backend->synchronize();

    for (int i = 0; i < WarmupIterations; ++i)
        REQUIRE(scheduler0->computeGraph(*graph0) == JobGgmlStatus::Success);

    for (int i = 0; i < WarmupIterations; ++i)
        REQUIRE(scheduler1->computeGraph(*graph1) == JobGgmlStatus::Success);

    cuda0Backend->synchronize();
    cuda1Backend->synchronize();

    BENCHMARK("GGML CUDA dual GPU independent mul_mat F16 16384^3 (x16 per GPU)") {
        std::atomic<bool> start{false};
        std::atomic<bool> failed0{false};
        std::atomic<bool> failed1{false};

        std::thread worker0([&] {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int i = 0; i < BatchIterations; ++i) {
                if (scheduler0->computeGraph(*graph0) != JobGgmlStatus::Success) {
                    failed0.store(true, std::memory_order_release);
                    return;
                }
            }

            cuda0Backend->synchronize();
        });

        std::thread worker1([&] {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int i = 0; i < BatchIterations; ++i) {
                if (scheduler1->computeGraph(*graph1) != JobGgmlStatus::Success) {
                    failed1.store(true, std::memory_order_release);
                    return;
                }
            }

            cuda1Backend->synchronize();
        });

        start.store(true, std::memory_order_release);

        worker0.join();
        worker1.join();

        if (failed0.load(std::memory_order_acquire))
            FAIL("CUDA0 independent graph execution failed");

        if (failed1.load(std::memory_order_acquire))
            FAIL("CUDA1 independent graph execution failed");
    };
}

// I was testing saturation of the cards without threads.
#if 0

TEST_CASE("GGML CUDA multi GPU split mul_mat 4096 cubed FP16", "[ggml][device][cuda][multi_gpu][mul_mat][f16][benchmark]")
{
    constexpr std::int64_t M = 4096;
    constexpr std::int64_t K = 4096;
    constexpr std::int64_t N = 4096;

    constexpr int BatchIterations = 16;

    auto &manager = testDeviceManager();
    manager.scan();

    const auto cudaDevices = manager.cudaDevices();

    if (cudaDevices.size() < 2)
        SKIP("Multi-GPU CUDA test requires at least two CUDA devices");

    JobGgmlCuda *cuda0 = cudaDevices[0];
    JobGgmlCuda *cuda1 = cudaDevices[1];
    REQUIRE(cuda0 != nullptr);
    REQUIRE(cuda1 != nullptr);
    REQUIRE(cuda0->isValid());
    REQUIRE(cuda1->isValid());

    JobGgmlCpu *cpu = manager.cpuDevice();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->isValid());

    const JobGgmlBackend::Ptr cuda0Backend = cuda0->backend();
    const JobGgmlBackend::Ptr cuda1Backend = cuda1->backend();

    REQUIRE(cuda0Backend != nullptr);
    REQUIRE(cuda1Backend != nullptr);
    REQUIRE(cuda0Backend->isValid());
    REQUIRE(cuda1Backend->isValid());

    WARN("CUDA0: " << cuda0->dump());
    WARN("CUDA1: " << cuda1->dump());

    //
    // Equal weight split across the two CUDA devices.
    //
    const float tensorSplit[] {
        0.5f,
        0.5f
    };

    ggml_backend_buffer_type_t nativeSplitType = JobGgmlCuda::splitBufferType(0, tensorSplit);

    REQUIRE(nativeSplitType != nullptr);

    auto splitType = JobGgmlBackendBufferType::createShared(nativeSplitType);
    REQUIRE(splitType != nullptr);
    REQUIRE(splitType->isValid());

    auto context = createMetadataContext(1024);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    auto left = context->newTensor2d(JobGgmlType::F16, K, M);
    auto right = context->newTensor2d(JobGgmlType::F16, K, N);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    const std::size_t leftAllocationSize =splitType->allocationSize(*left);
    REQUIRE(leftAllocationSize > 0);

    JobGgmlBackendBuffer::Ptr splitBuffer{
        splitType->allocateBuffer(leftAllocationSize)
    };
    REQUIRE(splitBuffer != nullptr);
    REQUIRE(splitBuffer->isValid());

    auto splitAllocator = JobGgmlTensorAllocator::createUniq(splitBuffer);

    REQUIRE(splitAllocator != nullptr);
    REQUIRE(splitAllocator->isValid());

    REQUIRE(splitAllocator->allocate(*left) == JobGgmlStatus::Success);

    REQUIRE(left->tensor()->buffer != nullptr);
    ggml_backend_buffer_t splitNativeBuffer = left->tensor()->buffer;

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

    manager.resetScheduler();

    JobGgmlDeviceManager::Devices devices{ cuda0, cuda1, cpu };
    auto scheduler = manager.buildScheduler(devices);

    REQUIRE(scheduler != nullptr);
    REQUIRE(scheduler->isValid());

    // NOTE: Do NOT assign left to cuda0 here. It already owns a CUDA split buffer.
    scheduler->setTensorBackend(*right, *cuda0Backend);
    scheduler->setTensorBackend(*expression, *cuda0Backend);

    scheduler->splitGraph(*graph);
    REQUIRE(scheduler->allocateGraph(*graph));

    //
    // Prove the scheduler did not relocate the split tensor.
    //
    CHECK(left->tensor()->buffer == splitNativeBuffer);
    const std::size_t leftElements = static_cast<std::size_t>(M * K);
    const std::size_t rightElements = static_cast<std::size_t>(K * N);
    std::vector<ggml_fp16_t> a(leftElements, ggml_fp32_to_fp16(0.01f));
    std::vector<ggml_fp16_t> b(rightElements, ggml_fp32_to_fp16(0.01f));

    ggml_backend_tensor_set(left->tensor(),
                            a.data(),
                            0,
                            a.size() * sizeof(ggml_fp16_t));

    ggml_backend_tensor_set(right->tensor(),
                            b.data(),
                            0,
                            b.size() * sizeof(ggml_fp16_t));

    for (int i = 0; i < 16; ++i)
        REQUIRE( scheduler->computeGraph(*graph) == JobGgmlStatus::Success );

    cuda0Backend->synchronize();
    cuda1Backend->synchronize();

    BENCHMARK("GGML CUDA multi GPU mul_mat F16 4096^3 (batched x16)")
    {
        for (int i = 0; i < BatchIterations; ++i)
            if (scheduler->computeGraph(*graph) != JobGgmlStatus::Success)
                FAIL("Multi-GPU CUDA graph execution failed");

        cuda0Backend->synchronize();
        cuda1Backend->synchronize();
    };
}

#endif // disabled split saturation experiment

#endif // JOB_TEST_BENCHMARKS