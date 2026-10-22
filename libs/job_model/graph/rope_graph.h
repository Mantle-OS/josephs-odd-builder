#pragma once

#include <cstdint>

#include <job_ggml_tensor.h>
#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"
#include "rope_config.h"

#include <contracts>

namespace job::model {
// we already(or should) a have a config for this why in the heck is this inherit of the config itsself

class JOBMODEL_EXPORT RopeGraph final
{
public:
    RopeGraph() = delete;
    ~RopeGraph() = delete;

    RopeGraph(const RopeGraph &) = delete;
    RopeGraph &operator=(const RopeGraph &) = delete;
    RopeGraph(RopeGraph &&) = delete;
    RopeGraph &operator=(RopeGraph &&) = delete;

    // rope dim and mode should be in the config
    // uint32_t ropeDimensions,
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &positions,
                                                           const RopeConfig *config);
};

} // namespace job::model





