#pragma once

#include <memory>

#include "jobggml_export.h"
namespace job::ggml {

/* Notes
 * eps: stands for "epsilon"
 * wd: stands for "weight decay"
 * Upstream stores these values in an anonymous nested structure inside ggml_opt_optimizer_params. This class gives that unnamed value group a strongly typed C++ representation.
 * learn more: Loshchilov & Hutter: https://arxiv.org/abs/1711.05101
 */

class JOBGGML_EXPORT JobGgmlOptAdamWParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptAdamWParams>;
    using UPtr = std::unique_ptr<JobGgmlOptAdamWParams>;

    explicit JobGgmlOptAdamWParams(float alpha = 0.001f, float beta1 = 0.9f, float beta2 = 0.999f, float eps = 1.0e-8f, float wd = 0.0f);
    ~JobGgmlOptAdamWParams() = default;

    [[nodiscard]] static Ptr createShared( float alpha = 0.001f, float beta1 = 0.9f, float beta2 = 0.999f, float eps = 1.0e-8f, float wd = 0.0f)
    {
        return std::make_shared<JobGgmlOptAdamWParams>(alpha, beta1, beta2, eps, wd);
    }

    [[nodiscard]] static UPtr createUniq( float alpha = 0.001f, float beta1 = 0.9f, float beta2 = 0.999f, float eps = 1.0e-8f, float wd = 0.0f)
    {
        return std::make_unique<JobGgmlOptAdamWParams>(alpha, beta1, beta2, eps, wd);
    }

    JobGgmlOptAdamWParams(const JobGgmlOptAdamWParams &) = delete;
    JobGgmlOptAdamWParams &operator=(const JobGgmlOptAdamWParams &) = delete;
    JobGgmlOptAdamWParams(JobGgmlOptAdamWParams &&) = delete;
    JobGgmlOptAdamWParams &operator=(JobGgmlOptAdamWParams &&) = delete;

    [[nodiscard]] float alpha() const noexcept;
    void setAlpha(float alpha);

    [[nodiscard]] float beta1() const noexcept;
    void setBeta1(float beta1);

    [[nodiscard]] float beta2() const noexcept;
    void setBeta2(float beta2);

    [[nodiscard]] float eps() const noexcept;
    void setEps(float eps);

    [[nodiscard]] float wd() const noexcept;
    void setWd(float wd);

    [[nodiscard]] bool isValid() const noexcept;

    void reset() noexcept;

private:
    float m_alpha{0.001f};
    float m_beta1{0.9f};
    float m_beta2{0.999f};
    float m_eps{1.0e-8f};
    float m_wd{0.0f};
};

} // namespace job::ggml