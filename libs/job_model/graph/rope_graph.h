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
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &positions,
                                                           uint32_t ropeDimensions,
                                                           int mode = 2);


    // Should be renamed to ropeExt as that is what is really going on here.
    // Fallback Dim and mode should be in the RopeConfig bottom line.
    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr build(ggml::JobGgmlTensorOp::UPtr input,
                                                           const ggml::JobGgmlTensor &positions,
                                                           const RopeConfig *config,
                                                           uint32_t fallbackDimensions,
                                                           int mode = 2);

};

} // namespace job::model





