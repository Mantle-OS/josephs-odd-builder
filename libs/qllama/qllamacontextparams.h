#ifndef QLLAMACONTEXTPARAMS_H
#define QLLAMACONTEXTPARAMS_H

#include "qllamabase.h"
#include "qllamaenums.h"
#include <QObject>
#include <QQmlEngine>
#include <llama.h>

#include <property-macros.h>

class QLlamaContextParams : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT

    QP_RW(quint32, nCtx,           0) // 0 = pull direct default from model metadata
    QP_RW(quint32, nBatch,         512)
    QP_RW(quint32, nUbatch,        512)
    QP_RW(quint32, nSeqMax,        1)
    QP_RW(quint32, nRsSeq,         0)
    QP_RW(qint32,  nThreads,       4)
    QP_RW(qint32,  nThreadsBatch,  4)

    QP_RW(QLlamaEnums::QLlamaContextType,     ctxType,        QLlamaEnums::QLlamaContextTypeDefault)
    QP_RW(QLlamaEnums::QLlamaRopeScalingType, ropeScalingType,QLlamaEnums::QLlamaRopeScalingTypeUnspecified)
    QP_RW(QLlamaEnums::QLlamaPoolingType,     poolingType,    QLlamaEnums::QLlamaPoolingTypeUnspecified)
    QP_RW(QLlamaEnums::QLlamaAttentionType,   attentionType,  QLlamaEnums::QLlamaAttentionTypeUnspecified)
    QP_RW(QLlamaEnums::QLlamaFlashAttnType,   flashAttnType,  QLlamaEnums::QLlamaFlashAttnTypeAuto)

    QP_RW(double,  ropeFreqBase,   0.0)
    QP_RW(double,  ropeFreqScale,  0.0)
    QP_RW(double,  yarnExtFactor,  -1.0)
    QP_RW(double,  yarnAttnFactor, 1.0)
    QP_RW(double,  yarnBetaFast,   0.0)
    QP_RW(double,  yarnBetaSlow,   0.0)
    QP_RW(quint32, yarnOrigCtx,    0)

    QP_RW(bool,    embeddings,     false)
    QP_RW(bool,    offloadKqv,     true)
    QP_RW(bool,    noPerf,         false)
    QP_RW(bool,    opOffload,      true)
    QP_RW(bool,    swaFull,        false)
    QP_RW(bool,    kvUnified,      true)

public:
    explicit QLlamaContextParams(QObject *parent = nullptr) :
        QLlamaBase{parent},
        m_contextParams(llama_context_default_params())
    {
        connect(this, &QLlamaContextParams::nCtxChanged,            this, [this](quint32 val){ m_contextParams.n_ctx = val; });
        connect(this, &QLlamaContextParams::nBatchChanged,          this, [this](quint32 val){ m_contextParams.n_batch = val; });
        connect(this, &QLlamaContextParams::nUbatchChanged,         this, [this](quint32 val){ m_contextParams.n_ubatch = val; });
        connect(this, &QLlamaContextParams::nSeqMaxChanged,         this, [this](quint32 val){ m_contextParams.n_seq_max = val; });
        connect(this, &QLlamaContextParams::nRsSeqChanged,          this, [this](quint32 val){ m_contextParams.n_rs_seq = val; });
        connect(this, &QLlamaContextParams::nThreadsChanged,        this, [this](qint32 val){ m_contextParams.n_threads = val; });
        connect(this, &QLlamaContextParams::nThreadsBatchChanged,   this, [this](qint32 val){ m_contextParams.n_threads_batch = val; });
        connect(this, &QLlamaContextParams::ctxTypeChanged,         this, [this](QLlamaEnums::QLlamaContextType val){ m_contextParams.ctx_type = QLlamaEnums::llamaContextType(val); });
        connect(this, &QLlamaContextParams::ropeScalingTypeChanged, this, [this](QLlamaEnums::QLlamaRopeScalingType val){ m_contextParams.rope_scaling_type = QLlamaEnums::llamaRopeScalingType(val); });
        connect(this, &QLlamaContextParams::poolingTypeChanged,     this, [this](QLlamaEnums::QLlamaPoolingType val){ m_contextParams.pooling_type = QLlamaEnums::llamaPoolingType(val); });
        connect(this, &QLlamaContextParams::attentionTypeChanged,   this, [this](QLlamaEnums::QLlamaAttentionType val){ m_contextParams.attention_type = QLlamaEnums::llamaAttentionType(val); });
        connect(this, &QLlamaContextParams::flashAttnTypeChanged,   this, [this](QLlamaEnums::QLlamaFlashAttnType val){ m_contextParams.flash_attn_type = QLlamaEnums::llamaFlashAttnType(val); });
        connect(this, &QLlamaContextParams::ropeFreqBaseChanged,    this, [this](double val){ m_contextParams.rope_freq_base = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::ropeFreqScaleChanged,   this, [this](double val){ m_contextParams.rope_freq_scale = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::yarnExtFactorChanged,   this, [this](double val){ m_contextParams.yarn_ext_factor = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::yarnAttnFactorChanged,  this, [this](double val){ m_contextParams.yarn_attn_factor = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::yarnBetaFastChanged,    this, [this](double val){ m_contextParams.yarn_beta_fast = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::yarnBetaSlowChanged,    this, [this](double val){ m_contextParams.yarn_beta_slow = static_cast<float>(val); });
        connect(this, &QLlamaContextParams::yarnOrigCtxChanged,     this, [this](quint32 val){ m_contextParams.yarn_orig_ctx = val; });
        connect(this, &QLlamaContextParams::embeddingsChanged,      this, [this](bool val){ m_contextParams.embeddings = val; });
        connect(this, &QLlamaContextParams::offloadKqvChanged,      this, [this](bool val){ m_contextParams.offload_kqv = val; });
        connect(this, &QLlamaContextParams::noPerfChanged,          this, [this](bool val){ m_contextParams.no_perf = val; });
        connect(this, &QLlamaContextParams::opOffloadChanged,       this, [this](bool val){ m_contextParams.op_offload = val; });
        connect(this, &QLlamaContextParams::swaFullChanged,         this, [this](bool val){ m_contextParams.swa_full = val; });
        connect(this, &QLlamaContextParams::kvUnifiedChanged,       this, [this](bool val){ m_contextParams.kv_unified = val; });
    }

    ~QLlamaContextParams() override = default;

    // Output accessor to capture the synchronized backend state struct cleanly
    llama_context_params prepareContextParams() const { return m_contextParams; }

private:
    llama_context_params m_contextParams;
};

#endif // QLLAMACONTEXTPARAMS_H