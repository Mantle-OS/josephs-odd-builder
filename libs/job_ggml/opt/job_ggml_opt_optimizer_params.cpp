#include "job_ggml_opt_optimizer_params.h"

#include <utility>

namespace job::ggml {

JobGgmlOptOptimizerParams::JobGgmlOptOptimizerParams() :
    JobGgmlOptOptimizerParams{defaultOptimizerParams()}
{

}

JobGgmlOptOptimizerParams::JobGgmlOptOptimizerParams(const struct ggml_opt_optimizer_params &params) :
    m_adamw{JobGgmlOptAdamWParams::createUniq(params.adamw.alpha, params.adamw.beta1, params.adamw.beta2, params.adamw.eps, params.adamw.wd)},
    m_sgd{JobGgmlOptSgdParams::createUniq(params.sgd.alpha, params.sgd.wd)}
{

}

JobGgmlOptAdamWParams *JobGgmlOptOptimizerParams::adamw() noexcept
{
    return m_adamw.get();
}

const JobGgmlOptAdamWParams *JobGgmlOptOptimizerParams::adamw() const noexcept
{
    return m_adamw.get();
}

JobGgmlOptSgdParams *JobGgmlOptOptimizerParams::sgd() noexcept
{
    return m_sgd.get();
}

const JobGgmlOptSgdParams *JobGgmlOptOptimizerParams::sgd() const noexcept
{
    return m_sgd.get();
}

bool JobGgmlOptOptimizerParams::isValid() const noexcept
{
    return m_adamw &&
           m_adamw->isValid() &&
           m_sgd &&
           m_sgd->isValid();
}

void JobGgmlOptOptimizerParams::setOptimizerParams(const struct ggml_opt_optimizer_params &params)
{
    auto adamw = JobGgmlOptAdamWParams::createUniq(params.adamw.alpha,
                                                   params.adamw.beta1, params.adamw.beta2,
                                                   params.adamw.eps, params.adamw.wd
                                                   );

    auto sgd = JobGgmlOptSgdParams::createUniq(params.sgd.alpha, params.sgd.wd);

    m_adamw = std::move(adamw);
    m_sgd   = std::move(sgd);
}

struct ggml_opt_optimizer_params JobGgmlOptOptimizerParams::optimizerParams() const noexcept
{
    if (!isValid())
        return defaultOptimizerParams();

    return {
        {
            m_adamw->alpha(),
            m_adamw->beta1(),
            m_adamw->beta2(),
            m_adamw->eps(),
            m_adamw->wd()
        },
        {
            m_sgd->alpha(),
            m_sgd->wd()
        }
    };
}

void JobGgmlOptOptimizerParams::reset()
{
    setOptimizerParams(defaultOptimizerParams());
}

} // namespace job::ggml