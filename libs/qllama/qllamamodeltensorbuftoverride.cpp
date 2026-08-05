#include "qllamamodeltensorbuftoverride.h"

QLlamaModelTensorBuftOverride::QLlamaModelTensorBuftOverride(QObject *parent) :
    QLlamaBase{parent}
{
}

void QLlamaModelTensorBuftOverride::setModelTensorBuftOverride(const llama_model_tensor_buft_override &other)
{
    m_modelTensorBuftOverride = other;

    set_pattern(other.pattern ? QString::fromUtf8(other.pattern) : QString{});
    set_buft(reinterpret_cast<quintptr>(other.buft));
}

llama_model_tensor_buft_override QLlamaModelTensorBuftOverride::modelTensorBuftOverride()
{
    llama_model_tensor_buft_override ret{defaultModelTensorBuftOverride()};

    m_patternBuffer = get_pattern().toUtf8();
    ret.pattern = m_patternBuffer.isEmpty() ? nullptr : m_patternBuffer.constData();
    ret.buft    = reinterpret_cast<ggml_backend_buffer_type_t>(get_buft());
    m_modelTensorBuftOverride = ret;
    return m_modelTensorBuftOverride;
}

void QLlamaModelTensorBuftOverride::resetModelTensorBuftOverride()
{
    m_patternBuffer.clear();
    m_modelTensorBuftOverride = defaultModelTensorBuftOverride();
    setModelTensorBuftOverride(m_modelTensorBuftOverride);
}
