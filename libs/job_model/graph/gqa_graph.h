#pragma once

#include <cstdint>

#include <job_ggml_tensor_op.h>

#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT GqaGraph final
{
public:
    GqaGraph() = delete;
    ~GqaGraph() = delete;

    GqaGraph(const GqaGraph &) = delete;
    GqaGraph &operator=(const GqaGraph &) = delete;
    GqaGraph(GqaGraph &&) = delete;
    GqaGraph &operator=(GqaGraph &&) = delete;

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr expand(ggml::JobGgmlTensorOp::UPtr input,
                                                            uint32_t queryHeadCount);
};

} // namespace job::model