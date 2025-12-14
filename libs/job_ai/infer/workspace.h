#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include "aligned_allocator.h"

namespace job::ai::infer {

#ifndef JOB_DEFAULT_WS_MB
#define JOB_DEFAULT_WS_MB 256
#endif
class Workspace {
public:
    explicit Workspace(std::size_t sizeBytes = JOB_DEFAULT_WS_MB * 1024ull * 1024ull) {
        resize(sizeBytes);
    }

    // VIEW ctor (non-owning)
    Workspace(float* base, std::size_t sizeBytes) noexcept :
        m_isView(true),
        m_view(base),
        m_viewBytes(sizeBytes)
    {
    }

    void resize(std::size_t sizeBytes) {
        if (m_isView)
            return; // or assert(false)
        const std::size_t count = (sizeBytes + sizeof(float) - 1) / sizeof(float);
        if (m_memory.size() != count)
            m_memory.resize(count);
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_isView ? m_viewBytes : (m_memory.size() * sizeof(float));
    }

    [[nodiscard]] std::size_t floatCount() const noexcept
    {
        return size() / sizeof(float);
    }
    float *raw() noexcept
    {
        return m_isView ? m_view : m_memory.data();
    }
    const float *raw() const noexcept
    {
        return m_isView ? m_view : m_memory.data();
    }



    // float *bufferA() noexcept { return m_memory.data(); }

    // float *bufferB() noexcept { return m_memory.data() + (m_memory.size() / 2); }


private:
    bool isView() const noexcept
    {
        return m_view != nullptr;
    }

    void makeOwning(std::size_t sizeBytes)
    {
        const std::size_t count = (sizeBytes + sizeof(float) - 1) / sizeof(float);
        if (m_memory.size() != count) m_memory.resize(count);
        m_view = nullptr;
        m_viewBytes = 0;
    }


    bool            m_isView{false};
    float           *m_view{nullptr};
    std::size_t     m_viewBytes{0};

    std::vector<float, job::ai::cords::AlignedAllocator<float, 64>> m_memory;

};

} // namespace job::ai::infer
