#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <ggml.h>

#include "jobggml_export.h"

namespace job::ggml {

// used in two places ... so far
// 1) device/impl/job_ggml_cpu
// 2) backend/job_ggml_backend once exposed if ever
using JobGgmlAbortCallbackFn = std::function<bool(void *)>;

class JOBGGML_EXPORT JobGgmlAbortCallback
{
public:
    using Ptr  = std::shared_ptr<JobGgmlAbortCallback>;
    using WPtr = std::weak_ptr<JobGgmlAbortCallback>;
    using UPtr = std::unique_ptr<JobGgmlAbortCallback>;

    explicit JobGgmlAbortCallback(JobGgmlAbortCallbackFn callback, void *userData = nullptr);

    ~JobGgmlAbortCallback() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlAbortCallbackFn callback, void *userData = nullptr)
    {
        return std::make_shared<JobGgmlAbortCallback>(std::move(callback), userData);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlAbortCallbackFn callback, void *userData = nullptr)
    {
        return std::make_unique<JobGgmlAbortCallback>(std::move(callback), userData);
    }

    JobGgmlAbortCallback(const JobGgmlAbortCallback &) = delete;
    JobGgmlAbortCallback &operator=(const JobGgmlAbortCallback &) = delete;
    JobGgmlAbortCallback(JobGgmlAbortCallback &&) = delete;
    JobGgmlAbortCallback &operator=(JobGgmlAbortCallback &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] bool invoke() const;

    [[nodiscard]] void *userData() const noexcept;
    void setUserData(void *userData) noexcept;

    [[nodiscard]] ggml_abort_callback callback() const noexcept { return callBouncer; }
    [[nodiscard]] void *callbackData() noexcept { return this; }

private:
    [[nodiscard]] static bool callBouncer(void *data);

    JobGgmlAbortCallbackFn  m_callback;
    void                    *m_userData{nullptr}; // Borrowed.
};

} // namespace job::ggml