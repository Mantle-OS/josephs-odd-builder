#pragma once

#include <cstdint>
#include <memory>

#include <job_ggml_context.h>
#include <job_ggml_cgraph.h>
#include <job_ggml_tensor.h>

#include "jobmodel_export.h"

namespace job::model {

// This should be a concept
class JOBMODEL_EXPORT GraphBuilder
{
public:
    using Ptr  = std::shared_ptr<GraphBuilder>;
    using WPtr = std::weak_ptr<GraphBuilder>;
    using UPtr = std::unique_ptr<GraphBuilder>;

    GraphBuilder() = default;
    virtual ~GraphBuilder() = default;

    GraphBuilder(const GraphBuilder &) = delete;
    GraphBuilder &operator=(const GraphBuilder &) = delete;
    GraphBuilder(GraphBuilder &&) noexcept = default;
    GraphBuilder &operator=(GraphBuilder &&) noexcept = default;

    [[nodiscard]] virtual ggml::JobGgmlCGraph::UPtr buildForward(ggml::JobGgmlContext &ctx,
                                                                 ggml::JobGgmlTensor &inputTokens,
                                                                 uint32_t nPast,
                                                                 ggml::JobGgmlType inputType = ggml::JobGgmlType::F16) = 0;


    // TODO buildBackwards().....
};

} // namespace job::model