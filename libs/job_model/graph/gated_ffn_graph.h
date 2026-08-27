#pragma once

#include <cstdint>

#include <job_ggml_enums.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"
#include "weights/model_weights.h"

namespace job::model {

// we already(or should) a have a config for this why in the heck is this inherit of the config itsself
class JOBMODEL_EXPORT GatedFfnGraph final
{
public:
    // TODO ADD THE REST These should really live in there own file not here.
    enum class Activation : uint8_t {
        Silu,
        Gelu,
        Relu
    };

    GatedFfnGraph() = delete;
    ~GatedFfnGraph() = delete;

    GatedFfnGraph(const GatedFfnGraph &) = delete;
    GatedFfnGraph &operator=(const GatedFfnGraph &) = delete;
    GatedFfnGraph(GatedFfnGraph &&) = delete;
    GatedFfnGraph &operator=(GatedFfnGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const LayerWeights &weights,
                                                           Activation activation = Activation::Silu,
                                                           ggml::JobGgmlType inputType = ggml::JobGgmlType::F16);

private:
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr activate(ggml::JobGgmlTensorOp::UPtr input, Activation activation);
};

} // namespace job::model