#pragma once

#include <cstddef>
namespace job::ai::infer {

class WorkspaceView {
public:
    WorkspaceView(float *base, std::size_t sizeBytes) :
        m_base(base),
        m_bytes(sizeBytes)
    {
    }

    std::size_t size() const noexcept
    {
        return m_bytes;
    }

    std::size_t floatCount() const noexcept
    {
        return m_bytes / sizeof(float);
    }
    float *raw() noexcept
    {
        return m_base;
    }
    const float* raw() const noexcept
    {
        return m_base;
    }

    void resize(std::size_t) { /* either no-op or assert(false) */ }

private:
    float *m_base{};
    std::size_t m_bytes{};
};

}
