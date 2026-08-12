#pragma once

#include <cstdint>
#include <memory>

#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlOptContext;
class JobGgmlOptDataset;
class JobGgmlOptResult;

/*
 * Describes progress reported during one training or validation subsection of
 * a high-level optimization epoch.
 *
 * JobGgmlOpt owns and updates this object while executing its JOB-managed
 * epoch and fit loops. Progress callbacks receive it as borrowed, read-only
 * state.
 *
 * context, dataset, and result are borrowed. Their owners must keep them alive
 * for the duration of the callback invocation.
 *
 * The fields map directly to ggml_opt_epoch_callback:
 * train: True after a training evaluation and false after validation evaluation.
 * ibatch: Number of batches evaluated so far in the current subsection.
 * ibatchMax: Total number of batches in the current subsection.
 * startTimeUs: Time at which evaluation of the current subsection began.
 */
class JOBGGML_EXPORT JobGgmlOptEpochProgress final
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptEpochProgress>;
    using UPtr = std::unique_ptr<JobGgmlOptEpochProgress>;

    explicit JobGgmlOptEpochProgress(bool train,
                                     JobGgmlOptContext *context,
                                     JobGgmlOptDataset *dataset,
                                     JobGgmlOptResult *result,
                                     std::int64_t ibatch,
                                     std::int64_t ibatchMax,
                                     std::int64_t startTimeUs
                                     );

    ~JobGgmlOptEpochProgress() = default;

    [[nodiscard]] static Ptr createShared(bool train,
                                          JobGgmlOptContext *context,
                                          JobGgmlOptDataset *dataset,
                                          JobGgmlOptResult *result,
                                          std::int64_t ibatch,
                                          std::int64_t ibatchMax,
                                          std::int64_t startTimeUs
                                          )
    {
        return std::make_shared<JobGgmlOptEpochProgress>(
            train,
            context,
            dataset,
            result,
            ibatch,
            ibatchMax,
            startTimeUs
            );
    }

    [[nodiscard]] static UPtr createUniq(bool train,
                                         JobGgmlOptContext *context,
                                         JobGgmlOptDataset *dataset,
                                         JobGgmlOptResult *result,
                                         std::int64_t ibatch,
                                         std::int64_t ibatchMax,
                                         std::int64_t startTimeUs)
    {
        return std::make_unique<JobGgmlOptEpochProgress>(
            train,
            context,
            dataset,
            result,
            ibatch,
            ibatchMax,
            startTimeUs
            );
    }

    JobGgmlOptEpochProgress(const JobGgmlOptEpochProgress &) = delete;
    JobGgmlOptEpochProgress &operator=(const JobGgmlOptEpochProgress &) = delete;
    JobGgmlOptEpochProgress(JobGgmlOptEpochProgress &&) = delete;
    JobGgmlOptEpochProgress &operator=(JobGgmlOptEpochProgress &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] bool train() const noexcept;
    [[nodiscard]] bool isTraining() const noexcept;
    [[nodiscard]] bool isValidation() const noexcept;

    [[nodiscard]] JobGgmlOptContext *context() noexcept;
    [[nodiscard]] const JobGgmlOptContext *context() const noexcept;

    [[nodiscard]] JobGgmlOptDataset *dataset() noexcept;
    [[nodiscard]] const JobGgmlOptDataset *dataset() const noexcept;

    [[nodiscard]] JobGgmlOptResult *result() noexcept;
    [[nodiscard]] const JobGgmlOptResult *result() const noexcept;

    [[nodiscard]] std::int64_t ibatch() const noexcept;
    [[nodiscard]] std::int64_t ibatchMax() const noexcept;
    [[nodiscard]] std::int64_t startTimeUs() const noexcept;

    [[nodiscard]] double progress() const noexcept;
    [[nodiscard]] bool isComplete() const noexcept;

    void update(
        bool train,
        JobGgmlOptContext *context,
        JobGgmlOptDataset *dataset,
        JobGgmlOptResult *result,
        std::int64_t ibatch,
        std::int64_t ibatchMax,
        std::int64_t startTimeUs
        );

private:
    bool               m_train{false};

    JobGgmlOptContext *m_context{nullptr}; // Borrowed
    JobGgmlOptDataset *m_dataset{nullptr}; // Borrowed
    JobGgmlOptResult  *m_result{nullptr};  // Borrowed

    std::int64_t       m_ibatch{0};
    std::int64_t       m_ibatchMax{0};
    std::int64_t       m_startTimeUs{0};
};

} // namespace job::ggml