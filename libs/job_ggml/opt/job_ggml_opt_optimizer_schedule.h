#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include <ggml-opt.h>

#include "job_ggml_opt_optimizer_params.h"
#include "jobggml_export.h"

namespace job::ggml {

/*
 * C++ callback bridge for ggml_opt_get_optimizer_params.
 *
 * The callback is invoked before an optimizer step and may update the supplied
 * parameter object according to call count or captured application state.
 *
 * callbackBouncer() is the native C entry point. It converts userData back to
 * this object, invokes the stored std::function, and returns native optimizer
 * parameters without allowing C++ exceptions to cross the C boundary.
 */
class JOBGGML_EXPORT JobGgmlOptOptimizerSchedule
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptOptimizerSchedule>;
    using WPtr = std::weak_ptr<JobGgmlOptOptimizerSchedule>;
    using UPtr = std::unique_ptr<JobGgmlOptOptimizerSchedule>;

    using Callback = std::function<void(JobGgmlOptOptimizerParams &params, std::int64_t callCount)>;

    explicit JobGgmlOptOptimizerSchedule(Callback callback);
    ~JobGgmlOptOptimizerSchedule() = default;

    [[nodiscard]] static Ptr createShared(Callback callback)
    {
        return std::make_shared<JobGgmlOptOptimizerSchedule>(std::move(callback));
    }

    [[nodiscard]] static UPtr createUniq(Callback callback)
    {
        return std::make_unique<JobGgmlOptOptimizerSchedule>(std::move(callback));
    }

    JobGgmlOptOptimizerSchedule(const JobGgmlOptOptimizerSchedule &) = delete;
    JobGgmlOptOptimizerSchedule &operator=(const JobGgmlOptOptimizerSchedule &) = delete;
    JobGgmlOptOptimizerSchedule(JobGgmlOptOptimizerSchedule &&) = delete;

    JobGgmlOptOptimizerSchedule &operator=(JobGgmlOptOptimizerSchedule &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] std::int64_t callCount() const noexcept;
    void resetCallCount() noexcept;
    void setCallback(Callback callback);

    [[nodiscard]] JobGgmlOptOptimizerParams *optimizerParams() noexcept;
    [[nodiscard]] const JobGgmlOptOptimizerParams *optimizerParams() const noexcept;

    [[nodiscard]] ggml_opt_get_optimizer_params nativeCallback() const noexcept;

    [[nodiscard]] void *nativeUserData() noexcept;
    [[nodiscard]] const void *nativeUserData() const noexcept;

private:
    [[nodiscard]] static struct ggml_opt_optimizer_params callbackBouncer(void *userData) noexcept;

    Callback                        m_callback;
    JobGgmlOptOptimizerParams::UPtr m_optimizerParams;
    std::int64_t                    m_callCount{0};
    struct ggml_opt_optimizer_params m_lastValidParams{};
};

} // namespace job::ggml