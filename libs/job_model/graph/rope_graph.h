#pragma once

#include <cstdint>

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"
#include "rope_config.h"

namespace job::model {

class JOBMODEL_EXPORT RopeGraph final
{
public:
    RopeGraph() = delete;
    ~RopeGraph() = delete;

    RopeGraph(const RopeGraph &) = delete;
    RopeGraph &operator=(const RopeGraph &) = delete;
    RopeGraph(RopeGraph &&) = delete;
    RopeGraph &operator=(RopeGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &positions,
                                                           uint32_t ropeDimensions,
                                                           int mode = 2);


    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &positions,
                                                           const RopeConfig *config,
                                                           uint32_t fallbackDimensions,
                                                           int mode = 2);

};

} // namespace job::model