#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>


#include <ggml.h>
#include <ggml-cpu.h>

#include "job_ggml_enums.h"
#include "jobggml_export.h"


#ifndef GGML_MAX_N_THREADS
#define GGML_MAX_N_THREADS 512
#endif

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlThreadPoolParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlThreadPoolParams>;
    using WPtr = std::weak_ptr<JobGgmlThreadPoolParams>;
    using UPtr = std::unique_ptr<JobGgmlThreadPoolParams>;

    explicit JobGgmlThreadPoolParams(int nThreads = recommendedThreadCount());
    explicit JobGgmlThreadPoolParams(const ggml_threadpool_params &params);
    ~JobGgmlThreadPoolParams() = default;

    [[nodiscard]] static Ptr createShared(int nThreads = recommendedThreadCount())
    {
        return std::make_shared<JobGgmlThreadPoolParams>(nThreads);
    }

    [[nodiscard]] static Ptr createShared(const ggml_threadpool_params &params)
    {
        return std::make_shared<JobGgmlThreadPoolParams>(params);
    }

    [[nodiscard]] static UPtr createUniq(int nThreads = recommendedThreadCount())
    {
        return std::make_unique<JobGgmlThreadPoolParams>(nThreads);
    }

    [[nodiscard]] static UPtr createUniq(const ggml_threadpool_params &params)
    {
        return std::make_unique<JobGgmlThreadPoolParams>(params);
    }

    JobGgmlThreadPoolParams(const JobGgmlThreadPoolParams &) = delete;
    JobGgmlThreadPoolParams &operator=(const JobGgmlThreadPoolParams &) = delete;
    JobGgmlThreadPoolParams(JobGgmlThreadPoolParams &&) = delete;
    JobGgmlThreadPoolParams &operator=(JobGgmlThreadPoolParams &&) = delete;

    [[nodiscard]] bool operator==(const JobGgmlThreadPoolParams &other) const noexcept;
    [[nodiscard]] bool operator!=(const JobGgmlThreadPoolParams &other) const noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    void reset() noexcept;

    [[nodiscard]] int nThreads() const noexcept;
    void setNThreads(int nThreads) noexcept;

    [[nodiscard]] JobGgmlSchedPriority prio() const noexcept;
    void setPrio(JobGgmlSchedPriority prio) noexcept;

    [[nodiscard]] std::uint32_t poll() const noexcept;
    void setPoll(std::uint32_t poll) noexcept;

    [[nodiscard]] bool strictCpu() const noexcept;
    void setStrictCpu(bool strictCpu) noexcept;

    [[nodiscard]] bool paused() const noexcept;
    void setPaused(bool paused) noexcept;

    [[nodiscard]] bool cpuEnabled(std::size_t index) const noexcept;
    void setCpuEnabled(std::size_t index, bool enabled) noexcept;
    void clearCpuMask() noexcept;

    void setParams(ggml_threadpool_params other) noexcept;
    [[nodiscard]] ggml_threadpool_params params() noexcept;
    void resetParams() noexcept;

    [[nodiscard]] static int recommendedThreadCount() noexcept
    {
        const auto count = std::thread::hardware_concurrency();
        return count == 0 ? 1 : std::min<int>(static_cast<int>(count), GGML_MAX_N_THREADS);
    }

private:
    // single thread by default
    [[nodiscard]] static constexpr ggml_threadpool_params defaultParams() noexcept
    {
        return {
            {},
            GGML_DEFAULT_N_THREADS,
            GGML_SCHED_PRIO_NORMAL,
            50,
            false,
            false
        };
    }

    ggml_threadpool_params m_params{defaultParams()}; // Upstream tracker.

    bool m_cpuMask[GGML_MAX_N_THREADS]{};
    int m_nThreads{recommendedThreadCount()};
    JobGgmlSchedPriority m_prio{JobGgmlSchedPriority::Normal};
    std::uint32_t m_poll{50};
    bool m_strictCpu{false};
    bool m_paused{false};
};

} // namespace job::ggml