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

    // Resize workspace to hold (sizeBytes / sizeof(float)) floats.
    // This may grow or shrink the logical size; capacity is managed by std::vector.
    void resize(std::size_t sizeBytes)
    {
        const std::size_t floatCount = sizeBytes / sizeof(float);

        if (m_memory.capacity() >= floatCount)
            return;

        m_memory.resize(floatCount);
    }

    float *bufferA() noexcept
    {
        return m_memory.data();
    }

    float *bufferB() noexcept
    {
        return m_memory.data() + (m_memory.size() / 2);
    }

    float *raw() noexcept
    {
        return m_memory.data();
    }

    const float *bufferA() const noexcept
    {
        return m_memory.data();
    }

    const float *bufferB() const noexcept
    {
        return m_memory.data() + (m_memory.size() / 2);
    }

    const float *raw() const noexcept
    {
        return m_memory.data();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_memory.size();
    }

private:
    std::vector<float, job::ai::cords::AlignedAllocator<float, 64>> m_memory;
};

} // namespace job::ai::infer
