#include "job_ggml_opt_step_info.h"

#include <limits>
#include <stdexcept>

namespace job::ggml {

JobGgmlOptStepInfo::JobGgmlOptStepInfo(std::int64_t epoch, std::int64_t optimizerStep, std::int64_t callbackCount) :
    m_epoch{epoch},
    m_optimizerStep{optimizerStep},
    m_callbackCount{callbackCount}
{
    if (!isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptStepInfo values must be greater than or equal to zero"
        };
    }
}

bool JobGgmlOptStepInfo::isValid() const noexcept
{
    return m_epoch >= 0 && m_optimizerStep >= 0 && m_callbackCount >= 0;
}

std::int64_t JobGgmlOptStepInfo::epoch() const noexcept
{
    return m_epoch;
}

void JobGgmlOptStepInfo::setEpoch(std::int64_t epoch)
{
    if (epoch < 0) {
        throw std::invalid_argument{
            "JobGgmlOptStepInfo epoch must be greater than or equal to zero"
        };
    }

    m_epoch = epoch;
}

std::int64_t JobGgmlOptStepInfo::optimizerStep() const noexcept
{
    return m_optimizerStep;
}

void JobGgmlOptStepInfo::setOptimizerStep(std::int64_t optimizerStep)
{
    if (optimizerStep < 0) {
        throw std::invalid_argument{
            "JobGgmlOptStepInfo optimizerStep must be greater than or equal to zero"
        };
    }

    m_optimizerStep = optimizerStep;
}

std::int64_t JobGgmlOptStepInfo::callbackCount() const noexcept
{
    return m_callbackCount;
}

void JobGgmlOptStepInfo::setCallbackCount(std::int64_t callbackCount)
{
    if (callbackCount < 0) {
        throw std::invalid_argument{
            "JobGgmlOptStepInfo callbackCount must be greater than or equal to zero"
        };
    }

    m_callbackCount = callbackCount;
}

bool JobGgmlOptStepInfo::incrementEpoch() noexcept
{
    return incrementSaturated(m_epoch);
}

bool JobGgmlOptStepInfo::incrementOptimizerStep() noexcept
{
    return incrementSaturated(m_optimizerStep);
}

bool JobGgmlOptStepInfo::incrementCallbackCount() noexcept
{
    return incrementSaturated(m_callbackCount);
}

void JobGgmlOptStepInfo::reset() noexcept
{
    m_epoch         = 0;
    m_optimizerStep = 0;
    m_callbackCount = 0;
}

bool JobGgmlOptStepInfo::incrementSaturated(std::int64_t &value) noexcept
{
    if (value >= std::numeric_limits<std::int64_t>::max())
        return false;
    ++value;
    return true;
}

} // namespace job::ggml