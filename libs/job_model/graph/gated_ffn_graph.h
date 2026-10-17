#pragma once

#include <cstdint>

#include <job_ggml_enums.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"
#include "job_activation.h"
#include "weights/model_weights.h"

namespace job::model {

// FIXME LATER we already(or should) a have a config for this why in the heck is this inherit of the config itsself
class JOBMODEL_EXPORT GatedFfnGraph final
{
public:
    GatedFfnGraph() = delete;
    ~GatedFfnGraph() = delete;

    GatedFfnGraph(const GatedFfnGraph &) = delete;
    GatedFfnGraph &operator=(const GatedFfnGraph &) = delete;
    GatedFfnGraph(GatedFfnGraph &&) = delete;
    GatedFfnGraph &operator=(GatedFfnGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const LayerWeights &weights,
                                                           ActivationType activation = ActivationType::Silu,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16);

};

} // namespace job::model