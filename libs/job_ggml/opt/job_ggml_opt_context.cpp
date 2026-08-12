#include "job_ggml_opt_context.h"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "job_ggml_cgraph.h"
#include "job_ggml_context.h"
#include "job_ggml_opt_params.h"
#include "job_ggml_opt_result.h"

namespace job::ggml {

JobGgmlOptContext::JobGgmlOptContext(const JobGgmlOptParams &params) :
    m_optimizerSchedule{params.optimizerSchedule()},
    m_optimizerPeriod{params.optPeriod()}
{
    if (!params.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptContext requires valid optimization parameters"
        };
    }

    if (!params.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptContext requires valid optimization parameters"
        };
    }

    m_context = ggml_opt_init(params.optParams());
    if (!m_context) {
        throw std::runtime_error{
            "Failed to initialize the GGML optimization context"
        };
    }

    m_graphsReady = ggml_opt_static_graphs(m_context);
}

JobGgmlOptContext::~JobGgmlOptContext()
{
    /*
     * The native context may retain m_optimizerSchedule as callback userdata.
     * Free it before releasing shared ownership of the schedule.
     */
    if (m_context) {
        ggml_opt_free(m_context);
        m_context = nullptr;
    }

    m_optimizerSchedule.reset();
}

bool JobGgmlOptContext::isValid() const noexcept
{
    return m_context != nullptr;
}

void JobGgmlOptContext::reset(bool optimizer)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot reset an invalid GGML optimization context"
        };
    }

    ggml_opt_reset(m_context, optimizer);

    if (optimizer && m_optimizerSchedule)
        m_optimizerSchedule->resetCallCount();
}

bool JobGgmlOptContext::usesStaticGraphs() const noexcept
{
    return m_context && ggml_opt_static_graphs(m_context);
}

JobGgmlOptOptimizerSchedule::Ptr JobGgmlOptContext::optimizerSchedule() const noexcept
{
    return m_optimizerSchedule;
}

JobGgmlTensor::UPtr JobGgmlOptContext::inputs() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_inputs(m_context);

    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlOptContext::outputs() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_outputs(m_context);
    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlOptContext::labels() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_labels(m_context);
    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlOptContext::loss() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_loss(m_context);

    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlOptContext::predictions() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_pred(m_context);
    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}

JobGgmlTensor::UPtr JobGgmlOptContext::correctCount() const
{
    if (!m_context)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_ncorrect(m_context);
    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}


JobGgmlTensor::UPtr JobGgmlOptContext::gradientAccumulator(JobGgmlTensor &node) const
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot inspect gradients from an invalid GGML optimization context"
        };
    }

    if (!node.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptContext::gradientAccumulator requires a valid tensor"
        };
    }

    if (!m_graphsReady)
        return nullptr;

    struct ggml_tensor *tensor = ggml_opt_grad_acc(m_context, node.tensor());
    return tensor ? JobGgmlTensor::createUniq(tensor) : nullptr;
}


JobGgmlOptOptimizerType JobGgmlOptContext::optimizerType() const noexcept
{
    return static_cast<JobGgmlOptOptimizerType>(ggmlOptimizerType());
}

enum ggml_opt_optimizer_type JobGgmlOptContext::ggmlOptimizerType() const noexcept
{
    if (!m_context)
        return GGML_OPT_OPTIMIZER_TYPE_COUNT;

    return ggml_opt_context_optimizer_type(m_context);
}

std::string_view JobGgmlOptContext::optimizerName() const noexcept
{
    if (!m_context)
        return "unknown";

    const enum ggml_opt_optimizer_type type = ggmlOptimizerType();
    if (type == GGML_OPT_OPTIMIZER_TYPE_COUNT)
        return "unknown";

    const char *name = ggml_opt_optimizer_name(type);
    return name ? std::string_view{name} : std::string_view{"unknown"};
}

int32_t JobGgmlOptContext::optimizerPeriod() const noexcept
{
    return m_optimizerPeriod;
}

void JobGgmlOptContext::prepareAlloc(
    JobGgmlContext &computeContext,
    JobGgmlCGraph &forwardGraph,
    JobGgmlTensor &inputs,
    JobGgmlTensor &outputs
    )
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot prepare an invalid GGML optimization context"
        };
    }

    if (usesStaticGraphs()) {
        throw std::logic_error{
            "JobGgmlOptContext::prepareAlloc cannot be used with static graphs"
        };
    }

    if (!computeContext.isValid() ||
        !forwardGraph.isValid() ||
        !inputs.isValid() ||
        !outputs.isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptContext::prepareAlloc requires valid context, graph, inputs, and outputs"
        };
    }

    ggml_opt_prepare_alloc(
        m_context,
        computeContext.context(),
        forwardGraph.graph(),
        inputs.tensor(),
        outputs.tensor()
        );

    // prepareAlloc supplies the dynamic forward graph, but ggml_opt_alloc() still has to build the gradient and optimizer graphs.
    m_graphsReady = false;
}

void JobGgmlOptContext::allocate(
    bool backward
    )
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot allocate an invalid GGML optimization context"
        };
    }

    ggml_opt_alloc(m_context, backward);
    m_graphsReady = true;
}

void JobGgmlOptContext::evaluate(JobGgmlOptResult *result)
{
    if (!m_context) {
        throw std::runtime_error{
            "Cannot evaluate an invalid GGML optimization context"
        };
    }

    if (result && !result->isValid()) {
        throw std::invalid_argument{
            "JobGgmlOptContext::evaluate received an invalid optimization result"
        };
    }

    ggml_opt_eval(m_context, result ? result->result() : nullptr);
}

ggml_opt_context_t JobGgmlOptContext::context() noexcept
{
    return m_context;
}

const struct ggml_opt_context *JobGgmlOptContext::context() const noexcept
{
    return m_context;
}

} // namespace job::ggml