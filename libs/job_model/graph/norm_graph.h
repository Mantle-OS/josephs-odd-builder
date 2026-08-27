#pragma once

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {
// we already(or should) a have a config for this why in the heck is this inherit of the config itsself

class JOBMODEL_EXPORT NormGraph final
{
public:
    NormGraph() = delete;
    ~NormGraph() = delete;

    NormGraph(const NormGraph &) = delete;
    NormGraph &operator=(const NormGraph &) = delete;
    NormGraph(NormGraph &&) = delete;
    NormGraph &operator=(NormGraph &&) = delete;


    // RMS is a okay name because that is what it does but need to talk about the full picure of this class
    // and the fact that everything else calls this generate.
    // This function needs contracts for pre and post.
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr rms(ggml::JobGgmlTensorOp::UPtr input,
                                                         const ggml::JobGgmlTensor &weight,
                                                         float eps,
                                                         const ggml::JobGgmlTensor *bias = nullptr);
};

} // namespace job::model