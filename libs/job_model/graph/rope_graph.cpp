#include "graph/rope_graph.h"

#include <limits>
#include <stdexcept>

namespace job::model {

ggml::JobGgmlTensorOp::UPtr RopeGraph::build(ggml::JobGgmlTensorOp::UPtr input,
                                             const ggml::JobGgmlTensor &positions,
                                             const RopeConfig *config)
{
    if (!config || !config->isValid())
        throw std::invalid_argument{"RopeGraph requires a valid config object"};

    if (!input || !input->isValid())
        throw std::invalid_argument{"RopeGraph requires a valid input tensor"};

    if (!positions.isValid())
        throw std::invalid_argument{"RopeGraph requires a valid position tensor"};

    switch (config->ropeType()) {
    case RopeType::Normal:
        return input->rope(positions, config->ropeDimCntInt(), config->modeInt());

    case RopeType::NormalInplace:
        return input->ropeInplace(positions, config->ropeDimCntInt(), config->modeInt());

    default:
        break;
    }

    // right here we are gonna need a fallback I bet=

    switch (config->ropeType()) {
    case RopeType::Normal:
    case RopeType::NormalInplace:
        break;

    case RopeType::Ext:
        return input->ropeExt(positions,
                              nullptr,
                              config->ropeDimCntInt(),
                              config->modeInt(),
                              config->ctxOrigCountInt(),
                              config->ropeFreqBase(),
                              config->ropeFreqScale(),
                              config->extFactor(),
                              config->attnFactor(),
                              config->betaFast(),
                              config->betaSlow());

    case RopeType::ExtInplace:
        return input->ropeExtInplace(positions,
                                     nullptr,
                                     config->ropeDimCntInt(),
                                     config->modeInt(),
                                     config->ctxOrigCountInt(),
                                     config->ropeFreqBase(),
                                     config->ropeFreqScale(),
                                     config->extFactor(),
                                     config->attnFactor(),
                                     config->betaFast(),
                                     config->betaSlow());

    case RopeType::Multi:
        return input->ropeMulti(positions,
                                nullptr,
                                config->ropeDimCntInt(),
                                const_cast<int *>(config->sections().data()),
                                config->modeInt(),
                                config->ctxOrigCountInt(),
                                config->ropeFreqBase(),
                                config->ropeFreqScale(),
                                config->extFactor(),
                                config->attnFactor(),
                                config->betaFast(),
                                config->betaSlow());

    case RopeType::MultiInplace:
        return input->ropeMultiInplace(positions,
                                       nullptr,
                                       config->ropeDimCntInt(),
                                       const_cast<int *>(config->sections().data()),
                                       config->modeInt(),
                                       config->ctxOrigCountInt(),
                                       config->ropeFreqBase(),
                                       config->ropeFreqScale(),
                                       config->extFactor(),
                                       config->attnFactor(),
                                       config->betaFast(),
                                       config->betaSlow());

    case RopeType::Custom:
        return input->ropeCustom(positions,
                                 config->ropeDimCntInt(),
                                 config->modeInt(),
                                 config->ctxOrigCountInt(),
                                 config->ropeFreqBase(),
                                 config->ropeFreqScale(),
                                 config->extFactor(),
                                 config->attnFactor(),
                                 config->betaFast(),
                                 config->betaSlow());

    case RopeType::CustomInplace:
        return input->ropeCustomInplace(positions,
                                        config->ropeDimCntInt(),
                                        config->modeInt(),
                                        config->ctxOrigCountInt(),
                                        config->ropeFreqBase(),
                                        config->ropeFreqScale(),
                                        config->extFactor(),
                                        config->attnFactor(),
                                        config->betaFast(),
                                        config->betaSlow());

    case RopeType::ExtBack:
        return input->ropeExtBack(positions,
                                  nullptr,
                                  config->ropeDimCntInt(),
                                  config->modeInt(),
                                  config->ctxOrigCountInt(),
                                  config->ropeFreqBase(),
                                  config->ropeFreqScale(),
                                  config->extFactor(),
                                  config->attnFactor(),
                                  config->betaFast(),
                                  config->betaSlow());
    case RopeType::MultiBack:
        return input->ropeMultiBack(positions,
                                    nullptr,
                                    config->ropeDimCntInt(),
                                    const_cast<int *>(config->sections().data()),
                                    config->modeInt(),
                                    config->ctxOrigCountInt(),
                                    config->ropeFreqBase(),
                                    config->ropeFreqScale(),
                                    config->extFactor(),
                                    config->attnFactor(),
                                    config->betaFast(),
                                    config->betaSlow());
    }

    throw std::invalid_argument{"Unsupported RoPE operation"};
}

} // namespace job::model