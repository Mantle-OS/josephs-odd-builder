#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QMap>
#include <QString>
#include <vector>

#include <llama.h>

#include <qllamamodelkvoverride.h>
#include "qllamabase.h"
#include "qllamaenums.h"

class QLlamaModelParams : public QLlamaBase
{
    Q_OBJECT

    // Core primitive tracking fields
    QP_RW(qint32, nGpuLayers,  99)
    QP_RW(QLlamaEnums::QLlamaSplitMode, splitMode, QLlamaEnums::QLlamaSplitModeNone)
    QP_RW(qint32, mainGpu,     0)

    // Dedicated single-override target reference
    QP_PTR_RO(QLlamaModelKvOverride, kvOverrides)

    // Aligned continuous boolean block
    QP_RW(bool, vocabOnly,      false)
    QP_RW(bool, useMmap,        true)
    QP_RW(bool, useDirectIO,    false)
    QP_RW(bool, useMLock,       false)
    QP_RW(bool, checkTensors,   false)
    QP_RW(bool, useExtraBufts,  false)
    QP_RW(bool, noHost,         false)
    QP_RW(bool, noAlloc,        false)

    QML_ELEMENT

public:
    explicit QLlamaModelParams(QObject *parent = nullptr) :
        QLlamaBase{parent},
        m_kvOverrides{new QLlamaModelKvOverride{this}},
        m_modelParams(llama_model_default_params())
    {
        // 1) Bind basic structural tracking properties directly down to the struct layout cache
        connect(this, &QLlamaModelParams::nGpuLayersChanged,   this, [this](qint32 val){ m_modelParams.n_gpu_layers = val; });
        connect(this, &QLlamaModelParams::splitModeChanged,    this, [this](QLlamaEnums::QLlamaSplitMode val){ m_modelParams.split_mode = static_cast<enum llama_split_mode>(val); });
        connect(this, &QLlamaModelParams::mainGpuChanged,      this, [this](qint32 val){ m_modelParams.main_gpu = val; });
        connect(this, &QLlamaModelParams::vocabOnlyChanged,    this, [this](bool val){ m_modelParams.vocab_only = val; });
        connect(this, &QLlamaModelParams::useMmapChanged,      this, [this](bool val){ m_modelParams.use_mmap = val; });
        connect(this, &QLlamaModelParams::useDirectIOChanged,  this, [this](bool val){ m_modelParams.use_direct_io = val; });
        connect(this, &QLlamaModelParams::useMLockChanged,     this, [this](bool val){ m_modelParams.use_mlock = val; });
        connect(this, &QLlamaModelParams::checkTensorsChanged, this, [this](bool val){ m_modelParams.check_tensors = val; });
        connect(this, &QLlamaModelParams::useExtraBuftsChanged,this, [this](bool val){ m_modelParams.use_extra_bufts = val; });
        connect(this, &QLlamaModelParams::noHostChanged,       this, [this](bool val){ m_modelParams.no_host = val; });
        connect(this, &QLlamaModelParams::noAllocChanged,      this, [this](bool val){ m_modelParams.no_alloc = val; });
    }

    ~QLlamaModelParams() override = default;

    // Q_INVOKABLE
    void appendtoBufferType(const QString &tensorPattern, ggml_backend_buffer_type_t buft)
    {
        if (!tensorPattern.isEmpty() && buft) {
            m_buftMap[tensorPattern] = buft;
        }
    }

    llama_model_params prepareParams()
    {
        m_kvBuffer.clear();
        m_kvBuffer.push_back(m_kvOverrides->nativeOverride());

        llama_model_kv_override kvTerminator{};
        kvTerminator.key[0] = '\0';
        m_kvBuffer.push_back(kvTerminator);
        m_modelParams.kv_overrides = m_kvBuffer.data();

        m_buftBuffer.clear();
        m_stringBackingStore.clear();

        if (!m_buftMap.isEmpty()) {
            m_buftBuffer.reserve(m_buftMap.size() + 1);
            m_stringBackingStore.reserve(m_buftMap.size());

            for (auto it = m_buftMap.constBegin(); it != m_buftMap.constEnd(); ++it) {
                m_stringBackingStore.push_back(it.key().toUtf8());
                llama_model_tensor_buft_override entry;
                entry.pattern = m_stringBackingStore.back().constData();
                entry.buft    = it.value();

                m_buftBuffer.push_back(entry);
            }

            // Append explicit structural terminator block
            llama_model_tensor_buft_override buftTerminator{};
            buftTerminator.pattern = nullptr; // Null pointer marks end of array
            buftTerminator.buft    = nullptr;
            m_buftBuffer.push_back(buftTerminator);

            m_modelParams.tensor_buft_overrides = m_buftBuffer.data();
        } else {
            m_modelParams.tensor_buft_overrides = nullptr;
        }

        return m_modelParams;
    }
private:
    llama_model_params m_modelParams;
    std::vector<llama_model_kv_override>            m_kvBuffer;
    std::vector<llama_model_tensor_buft_override>   m_buftBuffer;

    std::vector<QByteArray>                         m_stringBackingStore;

    QMap<QString, ggml_backend_buffer_type_t>       m_buftMap;
    ggml_backend_dev_t                             *m_devices = nullptr;
};
