#pragma once

#include "jobmodel_export.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <job_ggml_tensor_op.h>

namespace job::model {

enum class ActivationType : uint8_t {
    Identity   = 0,
    Sigmoid    = 1,
    Tanh       = 2,
    HardTanh   = 3,
    Relu       = 4,
    LeakyRelu  = 5,
    PRelu      = 6,
    RRelu      = 7,
    Elu        = 8,
    Selu       = 9,
    Gelu       = 10,
    AproxGelu  = 11,
    Swish      = 12,
    HSwish     = 13,
    Mish       = 14,
    HMish      = 15,
    Softplus   = 16,
    Maxout     = 17,
    Gdn        = 18,
    Silu       = 19,
    Unknown    = 254
};

class JOBMODEL_EXPORT JobActivation
{
public:
    using Ptr  = std::shared_ptr<JobActivation>;
    using WPtr = std::weak_ptr<JobActivation>;
    using UPtr = std::unique_ptr<JobActivation>;

    JobActivation() = delete;
    ~JobActivation() = delete;
    JobActivation(const JobActivation &) = delete;
    JobActivation &operator=(const JobActivation &) = delete;
    JobActivation(JobActivation &&) = delete;
    JobActivation &operator=(JobActivation &&) = delete;

    [[nodiscard]] constexpr static ActivationType fromStringView(std::string_view name)
    {
        if (name == "identity" || name == "linear")                                         return ActivationType::Identity;
        if (name == "sigmoid")                                                              return ActivationType::Sigmoid;
        if (name == "tanh")                                                                 return ActivationType::Tanh;
        if (name == "hardtanh" || name == "hard_tanh")                                      return ActivationType::HardTanh;
        if (name == "relu")                                                                 return ActivationType::Relu;
        if (name == "leaky_relu" || name == "leakyrelu")                                    return ActivationType::LeakyRelu;
        if (name == "prelu")                                                                return ActivationType::PRelu;
        if (name == "rrelu")                                                                return ActivationType::RRelu;
        if (name == "elu")                                                                  return ActivationType::Elu;
        if (name == "selu")                                                                 return ActivationType::Selu;
        if (name == "gelu")                                                                 return ActivationType::Gelu;
        if (name == "gelu_pytorch_tanh" || name == "gelu_fast" || name == "approx_gelu")    return ActivationType::AproxGelu;
        if (name == "swish")                                                                return ActivationType::Swish;
        if (name == "hardswish" || name == "hard_swish" || name == "hswish")                return ActivationType::HSwish;
        if (name == "mish")                                                                 return ActivationType::Mish;
        if (name == "hardmish" || name == "hard_mish" || name == "hmish")                   return ActivationType::HMish;
        if (name == "softplus")                                                             return ActivationType::Softplus;
        if (name == "maxout")                                                               return ActivationType::Maxout;
        if (name == "gdn")                                                                  return ActivationType::Gdn;
        if (name == "silu")                                                                 return ActivationType::Silu;

        // SwiGLU requires a separate gate tensor and is not a unary activation.
        throw std::invalid_argument{"JobActivation received an unknown activation name"};
    }

    [[nodiscard]] static ggml::JobGgmlTensorOp::UPtr activate(ggml::JobGgmlTensorOp::UPtr input,
                                                              ActivationType activation)
    {
        if (!input || !input->isValid())
            throw std::invalid_argument{"JobActivation requires a valid input tensor"};

        switch (activation) {
        case ActivationType::Identity:  return input;
        case ActivationType::Sigmoid:   return input->sigmoid();
        case ActivationType::Tanh:      return input->tanh();
        case ActivationType::HardTanh:  return input->clamp(-1.0f, 1.0f);
        case ActivationType::Relu:      return input->relu();
        case ActivationType::LeakyRelu: return input->leakyRelu(0.01f, false);
        case ActivationType::Elu:       return input->elu();
        case ActivationType::Gelu:      return input->gelu();
        case ActivationType::AproxGelu: return input->geluQuick();
        case ActivationType::Swish:
        case ActivationType::Silu:      return input->silu();
        case ActivationType::HSwish:    return input->hardswish();
        case ActivationType::Softplus:  return input->softplus();
        default:
            throw std::invalid_argument{"JobActivation requires additional parameters or an unsupported operation"};
        }
    }
};

} // namespace job::model