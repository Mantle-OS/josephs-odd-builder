#include "job_ggml_threadpool.h"

#include <stdexcept>

namespace job::ggml {

JobGgmlThreadPool::JobGgmlThreadPool(const JobGgmlThreadPoolParams &params) :
     m_nThreads{params.nThreads()}
{
    if (!params.isValid())
        throw std::invalid_argument{"JobGgmlThreadPool requires valid thread pool parameters"};

    ggml_threadpool_params nativeParams{};

    for (std::size_t i = 0; i < GGML_MAX_N_THREADS; ++i)
        nativeParams.cpumask[i] = params.cpuEnabled(i);

    nativeParams.n_threads  = params.nThreads();
    nativeParams.prio       = toGgmlSchedPriority(params.prio());
    nativeParams.poll       = params.poll();
    nativeParams.strict_cpu = params.strictCpu();
    nativeParams.paused     = params.paused();

    m_threadPool = ggml_threadpool_new(&nativeParams);

    if (!m_threadPool)
        throw std::runtime_error{"Failed to create GGML thread pool"};
}

JobGgmlThreadPool::~JobGgmlThreadPool()
{
    if (m_threadPool)
        ggml_threadpool_free(m_threadPool);

    m_threadPool = nullptr;
    // why we can not have nice things.
    m_nThreads = 0;
}

bool JobGgmlThreadPool::isValid() const noexcept
{
    return m_threadPool != nullptr;
}

int JobGgmlThreadPool::nThreads() const noexcept
{
    return m_threadPool ? m_nThreads : 0;
}

void JobGgmlThreadPool::pause() noexcept
{
    if (m_threadPool)
        ggml_threadpool_pause(m_threadPool);
}

void JobGgmlThreadPool::resume() noexcept
{
    if (m_threadPool)
        ggml_threadpool_resume(m_threadPool);
}

ggml_threadpool_t JobGgmlThreadPool::threadPool() noexcept
{
    return m_threadPool;
}

ggml_threadpool_t JobGgmlThreadPool::threadPool() const noexcept
{
    return m_threadPool;
}

} // namespace job::ggml