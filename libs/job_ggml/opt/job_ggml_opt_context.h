#pragma once

#include <memory>
#include <string_view>
#include <ggml-opt.h>

#include "job_ggml_enums.h"
#include "job_ggml_opt_optimizer_schedule.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlCGraph;
class JobGgmlContext;
class JobGgmlOptParams;
class JobGgmlOptResult;

class JOBGGML_EXPORT JobGgmlOptContext
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptContext>;
    using WPtr = std::weak_ptr<JobGgmlOptContext>;
    using UPtr = std::unique_ptr<JobGgmlOptContext>;

    explicit JobGgmlOptContext(const JobGgmlOptParams &params);
    ~JobGgmlOptContext();

    [[nodiscard]] static Ptr createShared(const JobGgmlOptParams &params)
    {
        return std::make_shared<JobGgmlOptContext>(params);
    }

    [[nodiscard]] static UPtr createUniq(const JobGgmlOptParams &params)
    {
        return std::make_unique<JobGgmlOptContext>(params);
    }

    JobGgmlOptContext(const JobGgmlOptContext &) = delete;
    JobGgmlOptContext &operator=(const JobGgmlOptContext &) = delete;
    JobGgmlOptContext(JobGgmlOptContext &&) = delete;
    JobGgmlOptContext &operator=(JobGgmlOptContext &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    void reset(bool optimizer = false);

    [[nodiscard]] bool usesStaticGraphs() const noexcept;

    [[nodiscard]] JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule() const noexcept;
    void setOptimizerSchedule(JobGgmlOptOptimizerSchedule::Ptr schedule);

    [[nodiscard]] JobGgmlTensor::UPtr inputs() const;
    [[nodiscard]] JobGgmlTensor::UPtr outputs() const;
    [[nodiscard]] JobGgmlTensor::UPtr labels() const;
    [[nodiscard]] JobGgmlTensor::UPtr loss() const;
    [[nodiscard]] JobGgmlTensor::UPtr predictions() const;
    [[nodiscard]] JobGgmlTensor::UPtr correctCount() const;
    [[nodiscard]] JobGgmlTensor::UPtr gradientAccumulator(JobGgmlTensor &node) const;

    [[nodiscard]] JobGgmlOptOptimizerType optimizerType() const noexcept;
    [[nodiscard]] enum ggml_opt_optimizer_type ggmlOptimizerType() const noexcept;

    [[nodiscard]] std::string_view optimizerName() const noexcept;
    std::int32_t optimizerPeriod() const noexcept;

    void prepareAlloc(JobGgmlContext &computeContext,
                      JobGgmlCGraph  &forwardGraph,
                      JobGgmlTensor  &inputs,
                      JobGgmlTensor  &outputs
                      );

    void allocate(bool backward);
    void evaluate(JobGgmlOptResult *result = nullptr);

    [[nodiscard]] ggml_opt_context_t context() noexcept;
    [[nodiscard]] const struct ggml_opt_context *context() const noexcept;

private:
    JobGgmlOptOptimizerSchedule::Ptr    m_optimizerSchedule;
    std::int32_t                        m_optimizerPeriod{1};
    ggml_opt_context_t                  m_context{nullptr}; // Owned
    bool                                m_graphsReady{false};
};

} // namespace job::ggml
