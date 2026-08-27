#pragma once

#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {
// we already(or should) a have a config for this why in the heck is this inherit of the config itsself

class JOBMODEL_EXPORT ResidualGraph final
{
public:
    ResidualGraph() = delete;
    ~ResidualGraph() = delete;

    ResidualGraph(const ResidualGraph &) = delete;
    ResidualGraph &operator=(const ResidualGraph &) = delete;
    ResidualGraph(ResidualGraph &&) = delete;
    ResidualGraph &operator=(ResidualGraph &&) = delete;

    // this needs pre frpom concepts
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           ggml::JobGgmlTensorOp::UPtr residual);
};

} // namespace job::model