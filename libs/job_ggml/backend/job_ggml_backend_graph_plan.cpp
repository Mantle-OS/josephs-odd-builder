#include "job_ggml_backend_graph_plan.h"

#include <stdexcept>

#include "job_ggml_backend.h"

namespace job::ggml {

JobGgmlBackendGraphPlan::JobGgmlBackendGraphPlan(JobGgmlBackend *backend, ggml_backend_graph_plan_t plan) :
    m_backend{backend},
    m_plan{plan}
{
    if (!m_backend || !m_backend->isValid())
        throw std::invalid_argument{ "JobGgmlBackendGraphPlan requires a valid JobGgmlBackend" };

    if (!m_plan)
        throw std::invalid_argument{ "JobGgmlBackendGraphPlan requires a valid GGML graph plan" };
}

JobGgmlBackendGraphPlan::~JobGgmlBackendGraphPlan()
{
    reset();
}

bool JobGgmlBackendGraphPlan::isValid() const noexcept
{
    return m_backend && m_backend->isValid() && m_plan;
}

void JobGgmlBackendGraphPlan::reset() noexcept
{
    if (m_backend && m_backend->isValid() && m_plan)
        ggml_backend_graph_plan_free(m_backend->backend(), m_plan);

    m_plan = nullptr;
}

ggml_backend_graph_plan_t JobGgmlBackendGraphPlan::plan() const noexcept
{
    return m_plan;
}

JobGgmlBackend *JobGgmlBackendGraphPlan::backend() noexcept
{
    return m_backend;
}

const JobGgmlBackend *JobGgmlBackendGraphPlan::backend() const noexcept
{
    return m_backend;
}

} // namespace job::ggml