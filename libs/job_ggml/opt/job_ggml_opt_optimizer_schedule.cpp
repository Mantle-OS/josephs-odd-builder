#include "job_ggml_opt_optimizer_schedule.h"

#include <limits>
#include <stdexcept>
#include <utility>

#ifndef NDEBUG
#include <job_logger.h>
#endif

namespace job::ggml {

JobGgmlOptOptimizerSchedule::JobGgmlOptOptimizerSchedule(Callback callback) :
    m_callback{std::move(callback)},
    m_optimizerParams{JobGgmlOptOptimizerParams::createUniq()}
{
    if (!m_callback) {
        throw std::invalid_argument{
            "JobGgmlOptOptimizerSchedule requires a valid callback"
        };
    }

    if (!m_optimizerParams || !m_optimizerParams->isValid()) {
        throw std::runtime_error{
            "Failed to initialize optimizer schedule parameters"
        };
    }

    m_lastValidParams = m_optimizerParams->optimizerParams();
}

bool JobGgmlOptOptimizerSchedule::isValid() const noexcept
{
    return static_cast<bool>(m_callback) &&
           m_optimizerParams &&
           m_optimizerParams->isValid() &&
           m_callCount >= 0;
}

std::int64_t JobGgmlOptOptimizerSchedule::callCount() const noexcept
{
    return m_callCount;
}

void JobGgmlOptOptimizerSchedule::resetCallCount() noexcept
{
    m_callCount = 0;
}

void JobGgmlOptOptimizerSchedule::setCallback(Callback callback)
{
    if (!callback) {
        throw std::invalid_argument{
            "JobGgmlOptOptimizerSchedule requires a valid callback"
        };
    }

    m_callback = std::move(callback);
}

JobGgmlOptOptimizerParams *JobGgmlOptOptimizerSchedule::optimizerParams() noexcept
{
    return m_optimizerParams.get();
}

const JobGgmlOptOptimizerParams *JobGgmlOptOptimizerSchedule::optimizerParams() const noexcept
{
    return m_optimizerParams.get();
}

ggml_opt_get_optimizer_params JobGgmlOptOptimizerSchedule::nativeCallback() const noexcept
{
    return &JobGgmlOptOptimizerSchedule::callbackBouncer;
}

void *JobGgmlOptOptimizerSchedule::nativeUserData() noexcept
{
    return this;
}

const void *JobGgmlOptOptimizerSchedule::nativeUserData() const noexcept
{
    return this;
}

struct ggml_opt_optimizer_params JobGgmlOptOptimizerSchedule::callbackBouncer(void *userData) noexcept
{
    if (!userData)
        return ggml_opt_get_default_optimizer_params(nullptr);

    auto *schedule = static_cast<JobGgmlOptOptimizerSchedule *>(userData);
    if (!schedule->m_callback || !schedule->m_optimizerParams)
        return schedule->m_lastValidParams;

    // restore....
    try {
        schedule->m_optimizerParams->setOptimizerParams(schedule->m_lastValidParams);
    } catch (...) {
        return schedule->m_lastValidParams;
    }

    // saturation check
    if (schedule->m_callCount < std::numeric_limits<std::int64_t>::max()) {
        ++schedule->m_callCount;
    } else {
#ifndef NDEBUG
        JOB_LOG_WARN("[JobGgmlOptOptimizerSchedule] Optimizer schedule call count  reached std::int64_t maximum and will remain saturated");
#endif
    }

    try {
        schedule->m_callback(*schedule->m_optimizerParams, schedule->m_callCount);

        if (!schedule->m_optimizerParams->isValid())
            return schedule->m_lastValidParams;

        schedule->m_lastValidParams = schedule->m_optimizerParams->optimizerParams();

        return schedule->m_lastValidParams;
    } catch (...) {
        /*
         * A C++ exception must never cross the native C callback boundary.
         * Return the most recent valid parameter set instead.
         */
        try {
            schedule->m_optimizerParams->setOptimizerParams(schedule->m_lastValidParams);
        } catch (...) {
            /*
             * Restoration can allocate and therefore theoretically fail.
             * m_lastValidParams itself remains valid and can still be safely
             * returned to GGML.
             */
        }

        return schedule->m_lastValidParams;
    }
}
} // namespace job::ggml