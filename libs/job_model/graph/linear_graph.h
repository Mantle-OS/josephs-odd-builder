#pragma once

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <job_ggml_enums.h>

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT LinearGraph final
{
public:
    LinearGraph() = delete;
    ~LinearGraph() = delete;

    LinearGraph(const LinearGraph &) = delete;
    LinearGraph &operator=(const LinearGraph &) = delete;
    LinearGraph(LinearGraph &&) = delete;
    LinearGraph &operator=(LinearGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &weight,
                                                           const ggml::JobGgmlTensor *bias = nullptr,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16);
};

} // namespace job::model