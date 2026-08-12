#pragma once

#include <memory>

#include <ggml-cpu.h>

#include "job_ggml_threadpool_params.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlThreadPool
{
public:
    using Ptr  = std::shared_ptr<JobGgmlThreadPool>;
    using WPtr = std::weak_ptr<JobGgmlThreadPool>;
    using UPtr = std::unique_ptr<JobGgmlThreadPool>;

    explicit JobGgmlThreadPool(const JobGgmlThreadPoolParams &params);
    ~JobGgmlThreadPool();

    [[nodiscard]] static Ptr createShared(const JobGgmlThreadPoolParams &params)
    {
        return std::make_shared<JobGgmlThreadPool>(params);
    }

    [[nodiscard]] static UPtr createUniq(const JobGgmlThreadPoolParams &params)
    {
        return std::make_unique<JobGgmlThreadPool>(params);
    }

    JobGgmlThreadPool(const JobGgmlThreadPool &) = delete;
    JobGgmlThreadPool &operator=(const JobGgmlThreadPool &) = delete;
    JobGgmlThreadPool(JobGgmlThreadPool &&) = delete;
    JobGgmlThreadPool &operator=(JobGgmlThreadPool &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] int nThreads() const noexcept;

    void pause() noexcept;
    void resume() noexcept;

    [[nodiscard]] ggml_threadpool_t threadPool() noexcept;
    [[nodiscard]] ggml_threadpool_t threadPool() const noexcept;

private:
    ggml_threadpool_t   m_threadPool{nullptr};  // Owned.
    int                 m_nThreads{0};          // ggml DECL but no IMPL workaround
};

} // namespace job::ggml