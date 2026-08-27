#pragma once

#include <cstdint>

#include <job_ggml_tensor.h>

#include "jobmodel_export.h"

namespace job::model {
struct JOBMODEL_EXPORT LayerWeights
{
    std::uint32_t layerIndex{0};

    ggml::JobGgmlTensor::UPtr attnNorm;
    ggml::JobGgmlTensor::UPtr attnNormBias;

    ggml::JobGgmlTensor::UPtr attnQ;
    ggml::JobGgmlTensor::UPtr attnK;
    ggml::JobGgmlTensor::UPtr attnV;
    ggml::JobGgmlTensor::UPtr attnOut;

    ggml::JobGgmlTensor::UPtr attnQBias;
    ggml::JobGgmlTensor::UPtr attnKBias;
    ggml::JobGgmlTensor::UPtr attnVBias;
    ggml::JobGgmlTensor::UPtr attnOutBias;

    ggml::JobGgmlTensor::UPtr attnQNorm;
    ggml::JobGgmlTensor::UPtr attnKNorm;

    ggml::JobGgmlTensor::UPtr postAttnNorm;

    ggml::JobGgmlTensor::UPtr ffnNorm;
    ggml::JobGgmlTensor::UPtr ffnNormBias;

    ggml::JobGgmlTensor::UPtr ffnGate;
    ggml::JobGgmlTensor::UPtr ffnUp;
    ggml::JobGgmlTensor::UPtr ffnDown;

    ggml::JobGgmlTensor::UPtr ffnGateBias;
    ggml::JobGgmlTensor::UPtr ffnUpBias;
    ggml::JobGgmlTensor::UPtr ffnDownBias;

    ggml::JobGgmlTensor::UPtr postFfnNorm;

    ggml::JobGgmlTensor::UPtr ffnGateInp;
    ggml::JobGgmlTensor::UPtr ffnGateExps;
    ggml::JobGgmlTensor::UPtr ffnUpExps;
    ggml::JobGgmlTensor::UPtr ffnDownExps;
};

}