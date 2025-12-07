#pragma once
#include <vector>
#include "aligned_allocator.h"

namespace job::ai::infer {

#ifndef JOB_DEFAULT_WS_MB
#define JOB_DEFAULT_WS_MB 256
#endif

class Workspace {
public:
    explicit Workspace(size_t sizeBytes = JOB_DEFAULT_WS_MB * 1024 * 1024)
    {
        size_t floatCount = sizeBytes / sizeof(float);
        m_memory.resize(floatCount);
        resize(sizeBytes);
    }

    void resize(size_t sizeBytes)
    {
        size_t floatCount = sizeBytes / sizeof(float);
        if (m_memory.capacity() >= floatCount)
            return;
        m_memory.resize(floatCount);
    }

    float *bufferA()
    {
        return m_memory.data();
    }
    float *bufferB()
    {
        return m_memory.data() + (m_memory.size() / 2);
    }
    float *raw()
    {
        return m_memory.data();
    }
    size_t size() const
    {
        return m_memory.size();
    }

private:
    std::vector<float, cords::AlignedAllocator<float, 64>> m_memory;
};

}
