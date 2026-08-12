#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <ggml-opt.h>

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlOptResult
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptResult>;
    using WPtr = std::weak_ptr<JobGgmlOptResult>;
    using UPtr = std::unique_ptr<JobGgmlOptResult>;

    JobGgmlOptResult();
    ~JobGgmlOptResult();

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGgmlOptResult>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGgmlOptResult>();
    }

    JobGgmlOptResult(const JobGgmlOptResult &) = delete;
    JobGgmlOptResult &operator=(const JobGgmlOptResult &) = delete;
    JobGgmlOptResult(JobGgmlOptResult &&) = delete;
    JobGgmlOptResult &operator=(JobGgmlOptResult &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;

    [[nodiscard]] std::int64_t ndata() const noexcept;
    [[nodiscard]] double loss(double *uncertainty = nullptr) const noexcept;
    [[nodiscard]] std::vector<std::int32_t> predictions() const;
    [[nodiscard]] double accuracy(double *uncertainty = nullptr) const noexcept;

    void reset() noexcept;

    [[nodiscard]] ggml_opt_result_t result() noexcept;
    [[nodiscard]] const struct ggml_opt_result *result() const noexcept;

private:
    ggml_opt_result_t m_result{nullptr}; // Owned
};

/*
 * Native ggml_opt_result members not exposed by the public API:
 * int64_t opt_period
 *     Captured from the optimization context during the first evaluation. Used internally when accumulated physical-batch losses represent per-datapoint loss.
 * bool loss_per_datapoint
 *     Captured from the optimization context during the first evaluation.  Controls how ggml_opt_result_loss() scales and aggregates loss.
 * Neither value can currently be queried through public ggml-opt APIs.
*/

} // namespace job::ggml