#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QMap>
#include <QString>
#include <vector>

#include <llama.h>


#include <property-macros.h>
#include <pointer-macros.h>
#include <objectmodel.h>

#include "qllamabase.h"
#include "qllamaenums.h"
#include "qllamamodelkvoverride.h"
#include "qllamamodeltensorbuftoverride.h"

class QLLAMA_EXPORT QLlamaModelParams : public QLlamaBase
{
    Q_OBJECT
    QP_PTR_RO(ObjectListModel<QLlamaBackendDevice>,             devices                                                 ) // Ordered runtime devices used for model offloading; empty lets llama.cpp use all available devices.
    QP_PTR_RO(ObjectListModel<QLlamaModelTensorBuftOverride>,   tensorBuftOverrides                                     ) // Tensor-pattern buffer-type overrides applied during model loading.
    QP_RW(qint32,                                               nGpuLayers,         0                                   ) // Number of model layers stored in VRAM; a negative value offloads every layer.
    QP_RW(QLlamaEnums::QLlamaSplitMode,                         splitMode,          QLlamaEnums::QLlamaSplitModeLayer   ) // Strategy used to distribute model tensors across multiple devices.
    QP_RW(qint32,                                               mainGpu,            0                                   ) // Device index holding the full model when splitMode is None.
    QP_RW(QList<float>,                                         tensorSplit,        QList<float>{}                      ) // Per-device model proportions; empty lets llama.cpp choose the distribution.
    QP_PTR_RO(ObjectListModel<QLlamaModelKvOverride>,           kvOverrides                                             ) // NULL-terminated model metadata override array reconstructed during native conversion.
    QP_RW(bool,                                                 vocabOnly,          false                               ) // Loads only vocabulary metadata and skips model weights.
    QP_RW(bool,                                                 useMmap,            true                                ) // Uses memory-mapped model loading when supported.
    QP_RW(bool,                                                 useDirectIO,        false                               ) // Uses direct I/O when supported and takes precedence over mmap.
    QP_RW(bool,                                                 useMLock,           false                               ) // Requests that loaded model memory remain locked in RAM.
    QP_RW(bool,                                                 checkTensors,       false                               ) // Validates model tensor data while loading.
    QP_RW(bool,                                                 useExtraBufts,      false                               ) // Enables additional buffer types used for weight repacking.
    QP_RW(bool,                                                 noHost,             false                               ) // Bypasses host buffers so additional backend buffers may be used.
    QP_RW(bool,                                                 noAlloc,            false                               ) // Loads metadata and simulates allocations without allocating model tensors.

    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(QLlamaModelParams)

public:
    explicit QLlamaModelParams(QObject *parent = nullptr);
    ~QLlamaModelParams() override = default;
    void setModelParams(const llama_model_params &other);
    [[nodiscard]] llama_model_params modelParams();
    void resetModelParams();

private:
    [[nodiscard]] static llama_model_params defaultModelParams() noexcept { return llama_model_default_params(); }
    llama_model_params                                  m_modelParams{defaultModelParams()};
    std::vector<ggml_backend_dev_t>                     m_deviceBuffer;
    std::vector<float>                                  m_tensorSplitBuffer;
    std::vector<llama_model_kv_override>                m_kvOverrideBuffer;
    std::vector<llama_model_tensor_buft_override>       m_tensorBuftOverrideBuffer;
    std::vector<QByteArray>                             m_tensorPatternBuffer;
};
