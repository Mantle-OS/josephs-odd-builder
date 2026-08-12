#include "job_ggml_opt_epoch_progress.h"

#include <stdexcept>

#include "job_ggml_opt_context.h"
#include "job_ggml_opt_dataset.h"
#include "job_ggml_opt_result.h"

namespace job::ggml {

JobGgmlOptEpochProgress::JobGgmlOptEpochProgress(bool train,
                                                 JobGgmlOptContext *context,
                                                 JobGgmlOptDataset *dataset,
                                                 JobGgmlOptResult *result,
                                                 std::int64_t ibatch,
                                                 std::int64_t ibatchMax,
                                                 std::int64_t startTimeUs)
{
    update(train,
           context,
           dataset,
           result,
           ibatch,
           ibatchMax,
           startTimeUs
           );
}

bool JobGgmlOptEpochProgress::isValid() const noexcept
{
    return m_context &&
           m_context->isValid() &&
           m_dataset &&
           m_dataset->isValid() &&
           m_result &&
           m_result->isValid() &&
           m_ibatch >= 0 &&
           m_ibatchMax >= 0 &&
           m_ibatch <= m_ibatchMax &&
           m_startTimeUs >= 0;
}

bool JobGgmlOptEpochProgress::train() const noexcept
{
    return m_train;
}

bool JobGgmlOptEpochProgress::isTraining() const noexcept
{
    return m_train;
}

bool JobGgmlOptEpochProgress::isValidation() const noexcept
{
    return !m_train;
}

JobGgmlOptContext *JobGgmlOptEpochProgress::context() noexcept
{
    return m_context;
}

const JobGgmlOptContext *JobGgmlOptEpochProgress::context() const noexcept
{
    return m_context;
}

JobGgmlOptDataset *JobGgmlOptEpochProgress::dataset() noexcept
{
    return m_dataset;
}

const JobGgmlOptDataset *JobGgmlOptEpochProgress::dataset() const noexcept
{
    return m_dataset;
}

JobGgmlOptResult *JobGgmlOptEpochProgress::result() noexcept
{
    return m_result;
}

const JobGgmlOptResult *JobGgmlOptEpochProgress::result() const noexcept
{
    return m_result;
}

std::int64_t JobGgmlOptEpochProgress::ibatch() const noexcept
{
    return m_ibatch;
}

std::int64_t JobGgmlOptEpochProgress::ibatchMax() const noexcept
{
    return m_ibatchMax;
}

std::int64_t JobGgmlOptEpochProgress::startTimeUs() const noexcept
{
    return m_startTimeUs;
}

double JobGgmlOptEpochProgress::progress() const noexcept
{
    if (m_ibatchMax <= 0)
        return 0.0;

    if (m_ibatch <= 0)
        return 0.0;

    if (m_ibatch >= m_ibatchMax)
        return 1.0;

    return static_cast<double>(m_ibatch) / static_cast<double>(m_ibatchMax);
}

bool JobGgmlOptEpochProgress::isComplete() const noexcept
{
    return m_ibatchMax > 0 && m_ibatch >= m_ibatchMax;
}

void JobGgmlOptEpochProgress::update(bool train,
                                     JobGgmlOptContext *context,
                                     JobGgmlOptDataset *dataset,
                                     JobGgmlOptResult *result,
                                     std::int64_t ibatch,
                                     std::int64_t ibatchMax,
                                     std::int64_t startTimeUs
                                     )
{
    if (!context || !context->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress requires a valid optimization context"
        };
    }

    if (!dataset || !dataset->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress requires a valid optimization dataset"
        };
    }

    if (!result || !result->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress requires a valid optimization result"
        };
    }

    if (ibatch < 0) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress ibatch must be greater than or equal to zero"
        };
    }

    if (ibatchMax < 0) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress ibatchMax must be greater than or equal to zero"
        };
    }

    if (ibatch > ibatchMax) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress ibatch cannot exceed ibatchMax"
        };
    }

    if (startTimeUs < 0) {
        throw std::invalid_argument{
            "JobGgmlOptEpochProgress startTimeUs must be greater than or equal to zero"
        };
    }
    m_train       = train;
    m_context     = context;
    m_dataset     = dataset;
    m_result      = result;
    m_ibatch      = ibatch;
    m_ibatchMax   = ibatchMax;
    m_startTimeUs = startTimeUs;
}

} // namespace job::ggml