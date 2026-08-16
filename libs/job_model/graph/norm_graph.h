#pragma once

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT NormGraph final
{
public:
    NormGraph() = delete;
    ~NormGraph() = delete;

    NormGraph(const NormGraph &) = delete;
    NormGraph &operator=(const NormGraph &) = delete;
    NormGraph(NormGraph &&) = delete;
    NormGraph &operator=(NormGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr rms(ggml::JobGgmlTensorOp::UPtr input,
                                                         const ggml::JobGgmlTensor &weight,
                                                         float eps,
                                                         const ggml::JobGgmlTensor *bias = nullptr);
};

} // namespace job::model