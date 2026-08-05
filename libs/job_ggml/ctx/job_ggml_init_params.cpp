#include "job_ggml_init_params.h"

namespace job::ggml {

JobGgmlInitParams::JobGgmlInitParams(const ggml_init_params &initParams)
{
    setInitParams(initParams);
}

std::size_t JobGgmlInitParams::memorySize() const noexcept
{
    return m_memorySize;
}

void JobGgmlInitParams::setMemorySize(std::size_t memorySize) noexcept
{
    if (m_memorySize != memorySize)
        m_memorySize = memorySize;
}

void *JobGgmlInitParams::memoryBuffer() const noexcept
{
    return m_memoryBuffer;
}

void JobGgmlInitParams::setMemoryBuffer(void *memoryBuffer) noexcept
{
    if (m_memoryBuffer != memoryBuffer)
        m_memoryBuffer = memoryBuffer;
}

bool JobGgmlInitParams::noAlloc() const noexcept
{
    return m_noAlloc;
}

void JobGgmlInitParams::setNoAlloc(bool noAlloc) noexcept
{
    if (m_noAlloc != noAlloc)
        m_noAlloc = noAlloc;
}

void JobGgmlInitParams::setInitParams(
    const ggml_init_params &other
    ) noexcept
{
    setMemorySize(other.mem_size);
    setMemoryBuffer(other.mem_buffer);
    setNoAlloc(other.no_alloc);

    m_initParams = other;
}

ggml_init_params JobGgmlInitParams::initParams() noexcept
{
    ggml_init_params ret{
        memorySize(),
        memoryBuffer(),
        noAlloc()
    };

    m_initParams = ret;

    return m_initParams;
}

void JobGgmlInitParams::resetInitParams() noexcept
{
    m_initParams = defaultInitParams();
    setInitParams(m_initParams);
}

} // namespace job::ggml