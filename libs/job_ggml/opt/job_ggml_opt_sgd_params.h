#pragma once

#include <memory>

#include "jobggml_export.h"

// Notes
// wd: stands for weight decay
// Upstream stores these values in an anonymous nested structure inside ggml_opt_optimizer_params. This class gives that unnamed value group a strongly typed C++ representation.
// Learn more: Robbins & Monro: https://doi.org/10.1214/aoms/1177729586
// Learn more: Bottou, Curtis & Nocedal: https://arxiv.org/abs/1606.04838

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlOptSgdParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptSgdParams>;
    using UPtr = std::unique_ptr<JobGgmlOptSgdParams>;

    explicit JobGgmlOptSgdParams(float alpha = 1.0e-3f, float wd    = 0.0f);
    ~JobGgmlOptSgdParams() = default;

    [[nodiscard]] static Ptr createShared(float alpha = 1.0e-3f, float wd = 0.0f)
    {
        return std::make_shared<JobGgmlOptSgdParams>(alpha, wd);
    }

    [[nodiscard]] static UPtr createUniq(float alpha = 1.0e-3f, float wd = 0.0f)
    {
        return std::make_unique<JobGgmlOptSgdParams>(alpha, wd);
    }

    JobGgmlOptSgdParams(const JobGgmlOptSgdParams &) = delete;
    JobGgmlOptSgdParams &operator=(const JobGgmlOptSgdParams &) = delete;
    JobGgmlOptSgdParams(JobGgmlOptSgdParams &&) = delete;
    JobGgmlOptSgdParams &operator=(JobGgmlOptSgdParams &&) = delete;

    [[nodiscard]] float alpha() const noexcept;
    void setAlpha(float alpha);

    [[nodiscard]] float wd() const noexcept;
    void setWd(float wd);

    [[nodiscard]] bool isValid() const noexcept;

    void reset() noexcept;

private:
    float m_alpha{1.0e-3f};
    float m_wd{0.0f};
};

} // namespace job::ggml