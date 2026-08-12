#pragma once

#include <cstdint>
#include <memory>

#include <ggml-opt.h>

#include "job_ggml_enums.h"
#include "job_ggml_opt_optimizer_schedule.h"
#include "job_ggml_opt_optimizer_schedule.h"

#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlBackendSched;
class JobGgmlContext;
class JobGgmlTensor;

class JOBGGML_EXPORT JobGgmlOptParams
{
public:
    using Ptr  = std::shared_ptr<JobGgmlOptParams>;
    using UPtr = std::unique_ptr<JobGgmlOptParams>;

    explicit JobGgmlOptParams(JobGgmlBackendSched *backendSched, JobGgmlOptLossType lossType);

    ~JobGgmlOptParams() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlBackendSched *backendSched, JobGgmlOptLossType lossType)
    {
        return std::make_shared<JobGgmlOptParams>(backendSched, lossType);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlBackendSched *backendSched, JobGgmlOptLossType lossType)
    {
        return std::make_unique<JobGgmlOptParams>(backendSched, lossType);
    }

    JobGgmlOptParams(const JobGgmlOptParams &) = delete;
    JobGgmlOptParams &operator=(const JobGgmlOptParams &) = delete;
    JobGgmlOptParams(JobGgmlOptParams &&) = delete;
    JobGgmlOptParams &operator=(JobGgmlOptParams &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool usesStaticGraphs() const noexcept;

    [[nodiscard]] JobGgmlBackendSched *backendSched() noexcept;
    [[nodiscard]] const JobGgmlBackendSched *backendSched() const noexcept;
    void setBackendSched(JobGgmlBackendSched *backendSched);

    [[nodiscard]] JobGgmlContext *computeContext() noexcept;
    [[nodiscard]] const JobGgmlContext *computeContext() const noexcept;
    void setComputeContext(JobGgmlContext *context) noexcept;

    [[nodiscard]] JobGgmlTensor *inputs() noexcept;
    [[nodiscard]] const JobGgmlTensor *inputs() const noexcept;
    void setInputs(JobGgmlTensor *inputs) noexcept;

    [[nodiscard]] JobGgmlTensor *outputs() noexcept;
    [[nodiscard]] const JobGgmlTensor *outputs() const noexcept;
    void setOutputs(JobGgmlTensor *outputs) noexcept;

    [[nodiscard]] JobGgmlOptLossType lossType() const noexcept;
    [[nodiscard]] enum ggml_opt_loss_type ggmlLossType() const noexcept;
    void setLossType(JobGgmlOptLossType lossType);
    void setGgmlLossType(enum ggml_opt_loss_type lossType);

    [[nodiscard]] JobGgmlOptOptimizerSchedule::Ptr optimizerSchedule() const noexcept;
    void setOptimizerSchedule(JobGgmlOptOptimizerSchedule::Ptr schedule);

    [[nodiscard]] JobGgmlOptBuildType buildType() const noexcept;
    [[nodiscard]] enum ggml_opt_build_type ggmlBuildType() const noexcept;
    void setBuildType(JobGgmlOptBuildType buildType);
    void setGgmlBuildType(enum ggml_opt_build_type buildType);

    [[nodiscard]] std::int32_t optPeriod() const noexcept;
    void setOptPeriod(std::int32_t optPeriod);

    [[nodiscard]] ggml_opt_get_optimizer_params getOptimizerParams() const noexcept;

    void setGetOptimizerParams(ggml_opt_get_optimizer_params callback) noexcept;
    [[nodiscard]] void *getOptimizerParamsUserData() const noexcept;

    void setGetOptimizerParamsUserData(void *userData) noexcept;
    void setGetOptimizerParams(ggml_opt_get_optimizer_params callback, void *userData) noexcept;

    [[nodiscard]] JobGgmlOptOptimizerType optimizer() const noexcept;
    [[nodiscard]] enum ggml_opt_optimizer_type ggmlOptimizer() const noexcept;

    void setOptimizer(JobGgmlOptOptimizerType optimizer);
    void setGgmlOptimizer(enum ggml_opt_optimizer_type optimizer);

    [[nodiscard]] struct ggml_opt_params optParams() const noexcept;

    void reset();

private:
    [[nodiscard]] static struct ggml_opt_params defaultOptParams(ggml_backend_sched_t backendSched, enum ggml_opt_loss_type lossType) noexcept;

    JobGgmlBackendSched                     *m_backendSched{nullptr};   // Borrowed
    JobGgmlContext                          *m_computeContext{nullptr}; // Borrowed
    JobGgmlTensor                           *m_inputs{nullptr};         // Borrowed
    JobGgmlTensor                           *m_outputs{nullptr};        // Borrowed
    JobGgmlOptOptimizerSchedule::Ptr        m_optimizerSchedule;
    JobGgmlOptLossType                      m_lossType{JobGgmlOptLossType::Mean}; // why are tou not Nice :P
    JobGgmlOptBuildType                     m_buildType{JobGgmlOptBuildType::Opt};
    std::int32_t                            m_optPeriod{1};
    ggml_opt_get_optimizer_params           m_getOptimizerParams{ggml_opt_get_default_optimizer_params};
    void                                    *m_getOptimizerParamsUserData{nullptr};
    JobGgmlOptOptimizerType                 m_optimizer{JobGgmlOptOptimizerType::AdamW}; // EveW :P
};
} // namespace job::ggml