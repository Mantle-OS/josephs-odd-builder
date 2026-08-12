#pragma once

#include <memory>

#include <ggml-opt.h>

#include "job_ggml_opt_adamw_params.h"
#include "job_ggml_opt_sgd_params.h"
#include "jobggml_export.h"

namespace job::ggml {

/*
 * Optimizer-specific parameter groups.
 *
 * Upstream stores the AdamW and SGD values as anonymous nested structures
 * inside ggml_opt_optimizer_params. This class owns the corresponding named
 * C++ parameter objects and provides conversion to and from the native
 * aggregate.
 */
class JOBGGML_EXPORT JobGgmlOptOptimizerParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptOptimizerParams>;
    using UPtr = std::unique_ptr<JobGgmlOptOptimizerParams>;

    JobGgmlOptOptimizerParams();

    explicit JobGgmlOptOptimizerParams(const struct ggml_opt_optimizer_params &params);
    ~JobGgmlOptOptimizerParams() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGgmlOptOptimizerParams>();
    }
    [[nodiscard]] static Ptr createShared(const struct ggml_opt_optimizer_params &params)
    {
        return std::make_shared<JobGgmlOptOptimizerParams>(params);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGgmlOptOptimizerParams>();
    }
    [[nodiscard]] static UPtr createUniq(const struct ggml_opt_optimizer_params &params)
    {
        return std::make_unique<JobGgmlOptOptimizerParams>(params);
    }

    JobGgmlOptOptimizerParams(const JobGgmlOptOptimizerParams &) = delete;
    JobGgmlOptOptimizerParams &operator=(const JobGgmlOptOptimizerParams &) = delete;
    JobGgmlOptOptimizerParams(JobGgmlOptOptimizerParams &&) = delete;
    JobGgmlOptOptimizerParams &operator=(JobGgmlOptOptimizerParams &&) = delete;

    [[nodiscard]] JobGgmlOptAdamWParams *adamw() noexcept;
    [[nodiscard]] const JobGgmlOptAdamWParams *adamw() const noexcept;

    [[nodiscard]] JobGgmlOptSgdParams *sgd() noexcept;
    [[nodiscard]] const JobGgmlOptSgdParams *sgd() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;

    void setOptimizerParams(const struct ggml_opt_optimizer_params &params);
    [[nodiscard]] struct ggml_opt_optimizer_params optimizerParams() const noexcept;

    void reset();

private:
    [[nodiscard]] static constexpr struct ggml_opt_optimizer_params defaultOptimizerParams() noexcept
    {
        return {
            {
                0.001f, // alpha: learning rate
                0.9f,   // beta1: first AdamW momentum
                0.999f, // beta2: second AdamW momentum
                1.0e-8f,// eps: numerical stability
                0.0f    // wd: weight decay; 0 disables it
            },
            {
                1.0e-3f, // alpha: learning rate
                0.0f     // wd: weight decay
            }
        };
    }
    JobGgmlOptAdamWParams::UPtr m_adamw;
    JobGgmlOptSgdParams::UPtr   m_sgd;
};

} // namespace job::ggml