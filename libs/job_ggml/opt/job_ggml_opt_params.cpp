#include "job_ggml_opt_params.h"

#include <stdexcept>
#include <utility>

#include "job_ggml_backend_sched.h"
#include "job_ggml_context.h"
#include "job_ggml_tensor.h"

namespace job::ggml {

JobGgmlOptParams::JobGgmlOptParams(JobGgmlBackendSched *backendSched, JobGgmlOptLossType lossType) :
    m_backendSched{backendSched},
    m_lossType{lossType}
{
    if (!m_backendSched || !m_backendSched->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptParams requires a valid JobGgmlBackendSched"
        };
    }

    // Validate the caller-provided loss type before asking upstream for its remaining defaults.
    setLossType(lossType);

    const struct ggml_opt_params defaults = defaultOptParams(m_backendSched->scheduler(), ggmlLossType());

    m_buildType = fromGgmlOptBuildType(defaults.build_type);

    m_optPeriod = defaults.opt_period;

    m_getOptimizerParams = defaults.get_opt_pars;

    m_getOptimizerParamsUserData = defaults.get_opt_pars_ud;

    m_optimizer = fromGgmlOptOptimizerType(defaults.optimizer);
}

bool JobGgmlOptParams::isValid() const noexcept
{
    if (!m_backendSched || !m_backendSched->isValid())
        return false;

    if (m_optPeriod <= 0)
        return false;

    if (!m_getOptimizerParams)
        return false;

    const bool hasAnyStaticGraphObject = m_computeContext ||
                                         m_inputs ||
                                         m_outputs;

    const bool hasAllStaticGraphObjects = m_computeContext &&
                                          m_inputs &&
                                          m_outputs;

    // Upstream treats ctx_compute, inputs, and outputs as one static-graph configuration. A partial set is not a usable configuration.
    if (hasAnyStaticGraphObject && !hasAllStaticGraphObjects)
        return false;

    if (m_computeContext && !m_computeContext->isValid())
        return false;

    if (m_inputs && !m_inputs->isValid())
        return false;

    if (m_outputs && !m_outputs->isValid())
        return false;

    return true;
}

bool JobGgmlOptParams::usesStaticGraphs() const noexcept
{
    return m_computeContext && m_inputs && m_outputs;
}

JobGgmlBackendSched *JobGgmlOptParams::backendSched() noexcept
{
    return m_backendSched;
}

const JobGgmlBackendSched *JobGgmlOptParams::backendSched() const noexcept
{
    return m_backendSched;
}

void JobGgmlOptParams::setBackendSched(JobGgmlBackendSched *backendSched)
{
    if (!backendSched || !backendSched->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptParams requires a valid JobGgmlBackendSched"
        };
    }

    m_backendSched = backendSched;
}

JobGgmlContext *JobGgmlOptParams::computeContext() noexcept
{
    return m_computeContext;
}

const JobGgmlContext *JobGgmlOptParams::computeContext() const noexcept
{
    return m_computeContext;
}

void JobGgmlOptParams::setComputeContext(JobGgmlContext *context) noexcept
{
    m_computeContext = context;
}

JobGgmlTensor *JobGgmlOptParams::inputs() noexcept
{
    return m_inputs;
}

const JobGgmlTensor *JobGgmlOptParams::inputs() const noexcept
{
    return m_inputs;
}

void JobGgmlOptParams::setInputs(JobGgmlTensor *inputs) noexcept
{
    m_inputs = inputs;
}

JobGgmlTensor *JobGgmlOptParams::outputs() noexcept
{
    return m_outputs;
}

const JobGgmlTensor *JobGgmlOptParams::outputs() const noexcept
{
    return m_outputs;
}

void JobGgmlOptParams::setOutputs(JobGgmlTensor *outputs) noexcept
{
    m_outputs = outputs;
}

JobGgmlOptLossType JobGgmlOptParams::lossType() const noexcept
{
    return m_lossType;
}

enum ggml_opt_loss_type JobGgmlOptParams::ggmlLossType() const noexcept
{
    return toGgmlOptLossType(m_lossType);
}

void JobGgmlOptParams::setLossType(JobGgmlOptLossType lossType)
{
    setGgmlLossType(toGgmlOptLossType(lossType));
}

void JobGgmlOptParams::setGgmlLossType(enum ggml_opt_loss_type lossType)
{
    switch (lossType) {
    case GGML_OPT_LOSS_TYPE_MEAN:
    case GGML_OPT_LOSS_TYPE_SUM:
    case GGML_OPT_LOSS_TYPE_CROSS_ENTROPY:
    case GGML_OPT_LOSS_TYPE_MEAN_SQUARED_ERROR:
        m_lossType = fromGgmlOptLossType(lossType);
        return;
    }

    throw std::invalid_argument{
        "JobGgmlOptParams received an invalid ggml_opt_loss_type"
    };
}

JobGgmlOptOptimizerSchedule::Ptr JobGgmlOptParams::optimizerSchedule() const noexcept
{
    return m_optimizerSchedule;
}

void JobGgmlOptParams::setOptimizerSchedule(JobGgmlOptOptimizerSchedule::Ptr schedule)
{
    if (schedule &&
        !schedule->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptParams received an invalid optimizer schedule"
        };
    }

    m_optimizerSchedule = std::move(schedule);
    if (m_optimizerSchedule) {
        m_getOptimizerParams = m_optimizerSchedule->nativeCallback();
        m_getOptimizerParamsUserData = m_optimizerSchedule->nativeUserData();
    } else {
        m_getOptimizerParams = ggml_opt_get_default_optimizer_params;
        m_getOptimizerParamsUserData = nullptr;
    }
}

JobGgmlOptBuildType JobGgmlOptParams::buildType() const noexcept
{
    return m_buildType;
}

enum ggml_opt_build_type JobGgmlOptParams::ggmlBuildType() const noexcept
{
    return toGgmlOptBuildType(m_buildType);
}

void JobGgmlOptParams::setBuildType(JobGgmlOptBuildType buildType)
{
    setGgmlBuildType(toGgmlOptBuildType(buildType));
}

void JobGgmlOptParams::setGgmlBuildType(enum ggml_opt_build_type buildType)
{
    switch (buildType) {
    case GGML_OPT_BUILD_TYPE_FORWARD:
    case GGML_OPT_BUILD_TYPE_GRAD:
    case GGML_OPT_BUILD_TYPE_OPT:
        m_buildType = fromGgmlOptBuildType(buildType);
        return;
    }

    throw std::invalid_argument{
        "JobGgmlOptParams received an invalid ggml_opt_build_type"
    };
}

std::int32_t JobGgmlOptParams::optPeriod() const noexcept
{
    return m_optPeriod;
}

void JobGgmlOptParams::setOptPeriod(std::int32_t optPeriod)
{
    if (optPeriod <= 0) {
        throw std::invalid_argument{
            "JobGgmlOptParams optPeriod must be greater than zero"
        };
    }

    m_optPeriod = optPeriod;
}

ggml_opt_get_optimizer_params JobGgmlOptParams::getOptimizerParams() const noexcept
{
    return m_getOptimizerParams;
}

void JobGgmlOptParams::setGetOptimizerParams(ggml_opt_get_optimizer_params callback) noexcept
{
    m_getOptimizerParams = callback;
}

void *JobGgmlOptParams::getOptimizerParamsUserData() const noexcept
{
    return m_getOptimizerParamsUserData;
}

void JobGgmlOptParams::setGetOptimizerParamsUserData(void *userData) noexcept
{
    m_getOptimizerParamsUserData = userData;
}

void JobGgmlOptParams::setGetOptimizerParams(ggml_opt_get_optimizer_params callback, void *userData) noexcept
{
    m_getOptimizerParams         = callback;
    m_getOptimizerParamsUserData = userData;
}

JobGgmlOptOptimizerType JobGgmlOptParams::optimizer() const noexcept
{
    return m_optimizer;
}

enum ggml_opt_optimizer_type JobGgmlOptParams::ggmlOptimizer() const noexcept
{
    return toGgmlOptOptimizerType(m_optimizer);
}

void JobGgmlOptParams::setOptimizer(JobGgmlOptOptimizerType optimizer)
{
    setGgmlOptimizer(toGgmlOptOptimizerType(optimizer));
}

void JobGgmlOptParams::setGgmlOptimizer(enum ggml_opt_optimizer_type optimizer)
{
    switch (optimizer) {
    case GGML_OPT_OPTIMIZER_TYPE_ADAMW:
    case GGML_OPT_OPTIMIZER_TYPE_SGD:
        m_optimizer = fromGgmlOptOptimizerType(
            optimizer
            );
        return;

    case GGML_OPT_OPTIMIZER_TYPE_COUNT:
        break;
    }

    throw std::invalid_argument{ "JobGgmlOptParams received an invalid ggml_opt_optimizer_type" };
}

struct ggml_opt_params JobGgmlOptParams::optParams() const noexcept
{
    return {
        m_backendSched ? m_backendSched->scheduler() : nullptr,
        m_computeContext ? m_computeContext->context() : nullptr,
        m_inputs ? m_inputs->tensor() : nullptr,
        m_outputs ? m_outputs->tensor() : nullptr,

        ggmlLossType(),
        ggmlBuildType(),

        m_optPeriod,

        m_getOptimizerParams,
        m_getOptimizerParamsUserData,

        ggmlOptimizer()
    };
}

void JobGgmlOptParams::reset()
{
    if (!m_backendSched || !m_backendSched->isValid()) {
        throw std::runtime_error{
            "Cannot reset JobGgmlOptParams without a valid backend scheduler"
        };
    }

    const struct ggml_opt_params defaults = defaultOptParams(m_backendSched->scheduler(), ggmlLossType());
    m_computeContext = nullptr;
    m_inputs         = nullptr;
    m_outputs        = nullptr;
    m_optimizerSchedule.reset();
    m_buildType = static_cast<JobGgmlOptBuildType>( defaults.build_type );
    m_optPeriod = defaults.opt_period;
    m_getOptimizerParams = defaults.get_opt_pars;
    m_getOptimizerParamsUserData = defaults.get_opt_pars_ud;
    m_optimizer = static_cast<JobGgmlOptOptimizerType>(defaults.optimizer);
}

struct ggml_opt_params JobGgmlOptParams::defaultOptParams(ggml_backend_sched_t backendSched, enum ggml_opt_loss_type lossType) noexcept
{
    return ggml_opt_default_params(backendSched, lossType);
}

} // namespace job::ggml