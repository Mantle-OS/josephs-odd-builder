#pragma once

#include <cstddef>
#include <vector>
#include "aligned_allocator.h"

namespace job::ai::infer {

#ifndef JOB_DEFAULT_WS_MB
#define JOB_DEFAULT_WS_MB 256
#endif

class Workspace {
public:
    explicit Workspace(std::size_t sizeBytes = JOB_DEFAULT_WS_MB * 1024ull * 1024ull)
    {
        resize(sizeBytes);
    }

    // Resize to hold at least 'sizeBytes'.
    // Aligns to float boundary.
    void resize(std::size_t sizeBytes)
    {
        // Round up to next float
        const std::size_t count = (sizeBytes + sizeof(float) - 1) / sizeof(float);

        if (m_memory.size() != count)
            m_memory.resize(count);
    }


    // Returns the total size in BYTES.
    // Matches ILayer::scratchSize().
    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_memory.size() * sizeof(float);
    }

    // Helper for math kernels that think in floats
    [[nodiscard]] std::size_t floatCount() const noexcept
    {
        return m_memory.size();
    }

    // Raw float pointer (Start of memory)
    float *raw() noexcept
    {
        return m_memory.data();
    }

    const float *raw() const noexcept
    {
        return m_memory.data();
    }

    // Ping-Pong Buffers (Half-Split)
    float *bufferA() noexcept
    {
        return m_memory.data();
    }

    float *bufferB() noexcept
    {
        // Pointer arithmetic on float* handles the sizeof automatically
        return m_memory.data() + (m_memory.size() / 2);
    }

    const float *bufferA() const noexcept
    {
        return m_memory.data();
    }

    const float *bufferB() const noexcept
    {
        return m_memory.data() + (m_memory.size() / 2);
    }

private:
    std::vector<float, job::ai::cords::AlignedAllocator<float, 64>> m_memory;
};

} // namespace job::ai::infer
