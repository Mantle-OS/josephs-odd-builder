#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

#include <ggml.h>

#include "job_ggml_context.h"
#include "job_ggml_cgraph.h"
#include "job_ggml_tensor_op.h"
#include "jobggml_export.h"

// AST side
namespace job::ggml {
class JOBGGML_EXPORT JobGgmlTensorOpGraph : public JobGgmlTensorOp
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorOpGraph>;
    using WPtr = std::weak_ptr<JobGgmlTensorOpGraph>;
    using UPtr = std::unique_ptr<JobGgmlTensorOpGraph>;

    explicit JobGgmlTensorOpGraph(struct ggml_tensor *tensor, JobGgmlContext *context) :
        JobGgmlTensorOp{tensor, context}
    {
        if (!context || !context->isValid())
            throw std::invalid_argument{"JobGgmlTensorOpGraph requires a valid JobGgmlContext"};
    }

    ~JobGgmlTensorOpGraph() = default;
    [[nodiscard]] static Ptr createShared(struct ggml_tensor *tensor, JobGgmlContext *context)
    {
        return std::make_shared<JobGgmlTensorOpGraph>(tensor, context);
    }

    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor, JobGgmlContext *context)
    {
        return std::make_unique<JobGgmlTensorOpGraph>(tensor, context);
    }

    JobGgmlTensorOpGraph(const JobGgmlTensorOpGraph &) = delete;
    JobGgmlTensorOpGraph &operator=(const JobGgmlTensorOpGraph &) = delete;
    JobGgmlTensorOpGraph(JobGgmlTensorOpGraph &&) = delete;
    JobGgmlTensorOpGraph &operator=(JobGgmlTensorOpGraph &&) = delete;

    [[nodiscard]] JobGgmlCGraph::UPtr buildGraph()
    {
        auto graph = context()->newGraph();
        if (!graph)
            throw std::runtime_error{"Failed to create JobGgmlCGraph"};

        graph->buildForwardExpand(*this);
        return graph;
    }

    [[nodiscard]] static UPtr wrap(JobGgmlTensorOp::UPtr tensorOp)
    {
        if (!tensorOp || !tensorOp->isValid() || !tensorOp->context())
            throw std::invalid_argument{"Cannot wrap an invalid JobGgmlTensorOp"};

        return createUniq(tensorOp->tensor(), tensorOp->context());
    }
};
} // namespace job::ggml