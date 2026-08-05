#pragma once

#include <QObject>
#include <QString>

#include <QQmlEngine>

#include <llama.h>

#include <property-macros.h>

#include "qllamabase.h"
#include "qllama_export.h"

class QLLAMA_EXPORT QLlamaModelTensorBuftOverride : public QLlamaBase
{
    Q_OBJECT
    QP_RW(QString,      pattern,    ""          ) // Tensor-name pattern selecting which tensors receive this buffer type.
    QP_RW(quintptr,     buft,       quintptr{}  ) // Opaque ggml backend buffer-type handle; runtime-only and not meaningful across processes.

    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(QLlamaModelTensorBuftOverride)

public:
    explicit QLlamaModelTensorBuftOverride(QObject *parent = nullptr);
    ~QLlamaModelTensorBuftOverride() override = default;

    void setModelTensorBuftOverride(const llama_model_tensor_buft_override &other);
    [[nodiscard]] llama_model_tensor_buft_override modelTensorBuftOverride();
    void resetModelTensorBuftOverride();

private:
    [[nodiscard]] static constexpr llama_model_tensor_buft_override defaultModelTensorBuftOverride() noexcept { return {nullptr, nullptr}; }
    llama_model_tensor_buft_override m_modelTensorBuftOverride{defaultModelTensorBuftOverride()};
    QByteArray                       m_patternBuffer;
};