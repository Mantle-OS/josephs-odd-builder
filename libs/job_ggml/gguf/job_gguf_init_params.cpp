#include "job_gguf_init_params.h"

namespace job::ggml {

JobGgufInitParams::JobGgufInitParams(bool noAlloc, bool createContext) noexcept :
    m_noAlloc{noAlloc},
    m_createContext{createContext}
{
    m_params = { m_noAlloc, nullptr };
}

JobGgufInitParams::JobGgufInitParams(bool noAlloc, JobGgmlContext::UPtr *contextOutput) noexcept :
    m_noAlloc{noAlloc},
    m_createContext{contextOutput != nullptr},
    m_contextOutput{contextOutput}
{
    m_params = { m_noAlloc, nullptr };
}

JobGgufInitParams::JobGgufInitParams(const struct gguf_init_params &initParams) noexcept
{
    setParams(initParams);
}

bool JobGgufInitParams::noAlloc() const noexcept
{
    return m_noAlloc;
}

void JobGgufInitParams::setNoAlloc(bool noAlloc) noexcept
{
    if (m_noAlloc == noAlloc)
        return;

    m_noAlloc = noAlloc;
    m_params.no_alloc = m_noAlloc;
}

bool JobGgufInitParams::createContext() const noexcept
{
    return m_createContext;
}

void JobGgufInitParams::setCreateContext(bool createContext) noexcept
{
    if (m_createContext == createContext)
        return;

    m_createContext = createContext;

    /*
     * The native output pointer is supplied only for the duration of a gguf_init_*() call, so m_params never retains it.
     */
    m_params.ctx = nullptr;

    if (!m_createContext)
        m_contextOutput = nullptr;
}

JobGgmlContext::UPtr *JobGgufInitParams::contextOutput() const noexcept
{
    return m_contextOutput;
}

void JobGgufInitParams::setContextOutput(JobGgmlContext::UPtr *contextOutput) noexcept
{
    m_contextOutput = contextOutput;
    m_createContext = contextOutput != nullptr;

    /*
     * JobGgmlContext::UPtr cannot be used as the native ggml_context **
     * output slot. JobGgufReader supplies that temporary native slot.
     */
    m_params.ctx = nullptr;
}

struct gguf_init_params JobGgufInitParams::params(struct ggml_context **contextOutput) const noexcept
{
    struct gguf_init_params ret{ m_noAlloc, nullptr };
    if (m_createContext)
        ret.ctx = contextOutput;

    return ret;
}

void JobGgufInitParams::setParams(const struct gguf_init_params &initParams) noexcept
{
    m_noAlloc       = initParams.no_alloc;
    m_createContext = initParams.ctx != nullptr;

    /*
     * A native ggml_context ** may refer to stack storage owned by the caller
     * of setParams(). Preserve its intent, but never retain the pointer.
     */
    m_contextOutput = nullptr;
    m_params = {m_noAlloc, nullptr};
}

void JobGgufInitParams::resetParams() noexcept
{
    m_params        = defaultParams();
    m_noAlloc       = m_params.no_alloc;
    m_createContext = false;
    m_contextOutput = nullptr;
}

} // namespace job::ggml