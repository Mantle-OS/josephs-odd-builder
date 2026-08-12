#pragma once

#include <cstdint>
#include <memory>

#include "jobggml_export.h"

namespace job::ggml {

/*
 * Describes the current position of a high-level optimization run.
 *
 * JobGgmlOpt owns and updates this object while executing its JOB-managed epoch and fit loops. Optimizer schedule callbacks receive it as borrowed, read-only state.
 * epoch: Current one-based dataset iteration.
 * optimizerStep:
 *     Number of completed optimizer updates. Gradient-only accumulation
 *     passes do not increment this value.
 * callbackCount: Number of times the optimizer-parameter schedule has been requested.
 */
class JOBGGML_EXPORT JobGgmlOptStepInfo final
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptStepInfo>;
    using UPtr = std::unique_ptr<JobGgmlOptStepInfo>;

    explicit JobGgmlOptStepInfo( std::int64_t epoch = 0, std::int64_t optimizerStep = 0, std::int64_t callbackCount = 0);
    ~JobGgmlOptStepInfo() = default;

    [[nodiscard]] static Ptr createShared(std::int64_t epoch = 0, std::int64_t optimizerStep = 0, std::int64_t callbackCount = 0)
    {
        return std::make_shared<JobGgmlOptStepInfo>(epoch, optimizerStep, callbackCount);
    }

    [[nodiscard]] static UPtr createUniq(std::int64_t epoch = 0, std::int64_t optimizerStep = 0, std::int64_t callbackCount = 0)
    {
        return std::make_unique<JobGgmlOptStepInfo>(epoch, optimizerStep, callbackCount);
    }

    JobGgmlOptStepInfo(const JobGgmlOptStepInfo &) = delete;
    JobGgmlOptStepInfo &operator=(const JobGgmlOptStepInfo &) = delete;
    JobGgmlOptStepInfo(JobGgmlOptStepInfo &&) = delete;
    JobGgmlOptStepInfo &operator=(JobGgmlOptStepInfo &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] std::int64_t epoch() const noexcept;
    void setEpoch(std::int64_t epoch);

    [[nodiscard]] std::int64_t optimizerStep() const noexcept;
    void setOptimizerStep(std::int64_t optimizerStep);

    [[nodiscard]] std::int64_t callbackCount() const noexcept;
    void setCallbackCount(std::int64_t callbackCount);

    [[nodiscard]] bool incrementEpoch() noexcept;
    [[nodiscard]] bool incrementOptimizerStep() noexcept;
    [[nodiscard]] bool incrementCallbackCount() noexcept;

    void reset() noexcept;

private:
    [[nodiscard]] static bool incrementSaturated(std::int64_t &value) noexcept;

    std::int64_t m_epoch{0};
    std::int64_t m_optimizerStep{0};
    std::int64_t m_callbackCount{0};
};

} // namespace job::ggml