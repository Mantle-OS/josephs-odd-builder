#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <cuda_runtime.h>
#include <vector>

// RAII Wrapper for a Pinned Memory Bridge
struct GpuBridge {
    float* pinned_ptr = nullptr;
    size_t n_elements;

    GpuBridge(size_t n) : n_elements(n) {
        // cudaHostAllocPortable allows any CUDA context to see this memory
        cudaHostAlloc(&pinned_ptr, n * sizeof(float), cudaHostAllocPortable);
    }

    ~GpuBridge() {
        cudaFreeHost(pinned_ptr);
    }
};

TEST_CASE("cuda: gpu bridge functional check", "[cuda][gpu_bridge]")
{

    int device_count = 0;
    cudaGetDeviceCount(&device_count);

    // const int device_count = 2; // We already verified this in previous logs
    const size_t n = 1024 * 1024; // 1M elements (4MB)

    GpuBridge bridge(n);
    float *d0, *d1;
    std::vector<float> h_src(n, 42.0f);
    std::vector<float> h_dst(n, 0.0f);

    // 1. Setup GPU 0 (Source)
    cudaSetDevice(0);
    cudaMalloc(&d0, n * sizeof(float));
    cudaMemcpy(d0, h_src.data(), n * sizeof(float), cudaMemcpyHostToDevice);

    // 2. Setup GPU 1 (Destination)
    cudaSetDevice(1);
    cudaMalloc(&d1, n * sizeof(float));

    SECTION("Synchronous Handoff (GPU 0 -> Bridge -> GPU 1)") {
        // Move GPU 0 to Bridge
        cudaSetDevice(0);
        cudaMemcpy(bridge.pinned_ptr, d0, n * sizeof(float), cudaMemcpyDeviceToHost);

        // Move Bridge to GPU 1
        cudaSetDevice(1);
        cudaMemcpy(d1, bridge.pinned_ptr, n * sizeof(float), cudaMemcpyHostToDevice);

        // Verify
        cudaMemcpy(h_dst.data(), d1, n * sizeof(float), cudaMemcpyDeviceToHost);
        REQUIRE(h_dst[0] == 42.0f);
        REQUIRE(h_dst[n-1] == 42.0f);
    }

    cudaSetDevice(0); cudaFree(d0);
    cudaSetDevice(1); cudaFree(d1);
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("cuda: gpu bridge benchmarks", "[cuda][gpu_bridge][bench]")
{
    const size_t n = 1024 * 1024 * 32; // 128MB payload
    GpuBridge bridge(n);
    float *d0, *d1;

    cudaSetDevice(0); cudaMalloc(&d0, n * sizeof(float));
    cudaSetDevice(1); cudaMalloc(&d1, n * sizeof(float));

    BENCHMARK("Handoff 128MB (GPU0 -> Pinned -> GPU1)") {
        cudaSetDevice(0);
        cudaMemcpy(bridge.pinned_ptr, d0, n * sizeof(float), cudaMemcpyDeviceToHost);
        cudaSetDevice(1);
        cudaMemcpy(d1, bridge.pinned_ptr, n * sizeof(float), cudaMemcpyHostToDevice);
        return cudaDeviceSynchronize();
    };

    cudaSetDevice(0); cudaFree(d0);
    cudaSetDevice(1); cudaFree(d1);
}
#endif