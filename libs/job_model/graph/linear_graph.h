#pragma once

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>
#include <job_ggml_enums.h>

#include "jobmodel_export.h"

namespace job::model {

// we already(or should) a have a config for this why in the heck is this inherit of the config itsself
class JOBMODEL_EXPORT LinearGraph final
{
public:
    LinearGraph() = delete;
    ~LinearGraph() = delete;

    LinearGraph(const LinearGraph &) = delete;
    LinearGraph &operator=(const LinearGraph &) = delete;
    LinearGraph(LinearGraph &&) = delete;
    LinearGraph &operator=(LinearGraph &&) = delete;


    // needs contracts for pre and post...
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &weight,
                                                           const ggml::JobGgmlTensor *bias = nullptr,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16);
};

} // namespace job::model