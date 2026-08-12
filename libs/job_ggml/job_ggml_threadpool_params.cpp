#include "job_ggml_threadpool_params.h"

namespace job::ggml {

JobGgmlThreadPoolParams::JobGgmlThreadPoolParams(int nThreads)
{
    setParams(defaultParams());
    setNThreads(nThreads);
}

JobGgmlThreadPoolParams::JobGgmlThreadPoolParams(const ggml_threadpool_params &params)
{
    setParams(params);
}

bool JobGgmlThreadPoolParams::operator==(const JobGgmlThreadPoolParams &other) const noexcept
{
    if (m_nThreads != other.m_nThreads || m_prio != other.m_prio || m_poll != other.m_poll || m_strictCpu != other.m_strictCpu || m_paused != other.m_paused)
        return false;

    for (std::size_t i = 0; i < GGML_MAX_N_THREADS; ++i) {
        if (m_cpuMask[i] != other.m_cpuMask[i])
            return false;
    }

    return true;
}

bool JobGgmlThreadPoolParams::operator!=(const JobGgmlThreadPoolParams &other) const noexcept
{
    return !(*this == other);
}

bool JobGgmlThreadPoolParams::isValid() const noexcept
{
    if (m_nThreads <= 0 || m_nThreads > GGML_MAX_N_THREADS)
        return false;

    if (m_poll > 100)
        return false;

    const auto priority = static_cast<std::int8_t>(m_prio);
    if (priority < static_cast<std::int8_t>(JobGgmlSchedPriority::Low) ||
        priority > static_cast<std::int8_t>(JobGgmlSchedPriority::Realtime))
        return false;

    return true;
}

void JobGgmlThreadPoolParams::reset() noexcept
{
    resetParams();
}

int JobGgmlThreadPoolParams::nThreads() const noexcept
{
    return m_nThreads;
}

void JobGgmlThreadPoolParams::setNThreads(int nThreads) noexcept
{
    if (m_nThreads != nThreads)
        m_nThreads = nThreads;
}

JobGgmlSchedPriority JobGgmlThreadPoolParams::prio() const noexcept
{
    return m_prio;
}

void JobGgmlThreadPoolParams::setPrio(JobGgmlSchedPriority prio) noexcept
{
    if (m_prio != prio)
        m_prio = prio;
}

std::uint32_t JobGgmlThreadPoolParams::poll() const noexcept
{
    return m_poll;
}

void JobGgmlThreadPoolParams::setPoll(std::uint32_t poll) noexcept
{
    if (m_poll != poll)
        m_poll = poll;
}

bool JobGgmlThreadPoolParams::strictCpu() const noexcept
{
    return m_strictCpu;
}

void JobGgmlThreadPoolParams::setStrictCpu(bool strictCpu) noexcept
{
    if (m_strictCpu != strictCpu)
        m_strictCpu = strictCpu;
}

bool JobGgmlThreadPoolParams::paused() const noexcept
{
    return m_paused;
}

void JobGgmlThreadPoolParams::setPaused(bool paused) noexcept
{
    if (m_paused != paused)
        m_paused = paused;
}

bool JobGgmlThreadPoolParams::cpuEnabled(std::size_t index) const noexcept
{
    if (index >= GGML_MAX_N_THREADS)
        return false;

    return m_cpuMask[index];
}

void JobGgmlThreadPoolParams::setCpuEnabled(std::size_t index, bool enabled) noexcept
{
    if (index >= GGML_MAX_N_THREADS)
        return;

    if (m_cpuMask[index] != enabled)
        m_cpuMask[index] = enabled;
}

void JobGgmlThreadPoolParams::clearCpuMask() noexcept
{
    for (std::size_t i = 0; i < GGML_MAX_N_THREADS; ++i)
        m_cpuMask[i] = false;
}

void JobGgmlThreadPoolParams::setParams(ggml_threadpool_params other) noexcept
{
    for (std::size_t i = 0; i < GGML_MAX_N_THREADS; ++i)
        setCpuEnabled(i, other.cpumask[i]);

    setNThreads(other.n_threads);
    setPrio(fromGgmlSchedPriority(other.prio));
    setPoll(other.poll);
    setStrictCpu(other.strict_cpu);
    setPaused(other.paused);

    m_params = other;
}

ggml_threadpool_params JobGgmlThreadPoolParams::params() noexcept
{
    ggml_threadpool_params ret{defaultParams()};

    for (std::size_t i = 0; i < GGML_MAX_N_THREADS; ++i)
        ret.cpumask[i] = cpuEnabled(i);

    ret.n_threads  = nThreads();
    ret.prio       = toGgmlSchedPriority(prio());
    ret.poll       = poll();
    ret.strict_cpu = strictCpu();
    ret.paused     = paused();

    m_params = ret;
    return m_params;
}

void JobGgmlThreadPoolParams::resetParams() noexcept
{
    setParams(defaultParams());
}

} // namespace job::ggml