#include <cstddef>
#include <cuda_runtime.h>

namespace job::cuda {

class GpuBridge {
public:
    GpuBridge(size_t buffer_size)
    {
        // Allocate memory that is accessible by all GPUs and the CPU
        cudaHostAlloc(&m_pinned_ptr, buffer_size, cudaHostAllocPortable);
    }

    ~GpuBridge()
    {
        cudaFreeHost(m_pinned_ptr);
    }

    // Asynchronously move data from GPU 0 to GPU 1 via the pinned bridge
    void transfer_async(void* src, void* dst, size_t size, cudaStream_t s0, cudaStream_t s1)
    {
        // GPU 0: Device -> Pinned RAM
        cudaMemcpyAsync(m_pinned_ptr, src, size, cudaMemcpyDeviceToHost, s0);
        // GPU 1: Pinned RAM -> Device (waits for GPU 0 to finish writing)
        cudaStreamWaitEvent(s1, m_transfer_event, 0);
        cudaMemcpyAsync(dst, m_pinned_ptr, size, cudaMemcpyHostToDevice, s1);
    }

private:
    void *m_pinned_ptr;
    cudaEvent_t m_transfer_event;
};

}