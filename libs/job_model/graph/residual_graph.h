#pragma once

#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT ResidualGraph final
{
public:
    ResidualGraph() = delete;
    ~ResidualGraph() = delete;

    ResidualGraph(const ResidualGraph &) = delete;
    ResidualGraph &operator=(const ResidualGraph &) = delete;
    ResidualGraph(ResidualGraph &&) = delete;
    ResidualGraph &operator=(ResidualGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           ggml::JobGgmlTensorOp::UPtr residual);
};

} // namespace job::model