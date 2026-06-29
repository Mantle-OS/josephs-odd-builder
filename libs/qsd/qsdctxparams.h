#ifndef QSDCTXPARAMS_H
#define QSDCTXPARAMS_H

#include <QObject>
#include <QQmlEngine>
#include <QDirIterator>
#include <QDir>
#include <stable-diffusion.h>

#include <pointer-macros.h>
#include <objectmodel.h>
#include <qaiutils.h>

#include "qsdbaseparam.h"
#include "qsdenums.h"
#include "qsdembedding.h"

class QSdCtxParams : public QSdBaseParam
{
    Q_OBJECT

    QP_RW(QString,                              modelPath,                      ""                              )
    QP_RW(QString,                              clipLPath,                      ""                              )
    QP_RW(QString,                              clipGPath,                      ""                              )
    QP_RW(QString,                              clipVisionPath,                 ""                              )
    QP_RW(QString,                              t5xxlPath,                      ""                              )
    QP_RW(QString,                              llmPath,                        ""                              )
    QP_RW(QString,                              llmVisionPath,                  ""                              )
    QP_RW(QString,                              diffusionModelPath,             ""                              )
    QP_RW(QString,                              highNoiseDiffusionModelPath,    ""                              )
    QP_RW(QString,                              uncondDiffusionModelPath,       ""                              )
    QP_RW(QString,                              embeddingsConnectorsPath,       ""                              )
    QP_RW(QString,                              vaePath,                        ""                              )
    QP_RW(QString,                              audioVaePath,                   ""                              )
    QP_RW(QString,                              taesdPath,                      ""                              )
    QP_RW(QString,                              controlNetPath,                 ""                              )
    QP_RW(QString,                              photoMakerPath,                 ""                              )
    //
    QP_RW(QString,                              tensorTypeRules,                ""                              )
    QP_RW(QString,                              backend,                        ""                              )
    QP_RW(QString,                              paramsBackend,                  ""                              )
    QP_RW(QString,                              rpcServers,                     ""                              )
    //
    QP_RW(QSdEnums::QSdWeightTypes,             weightType,                     QSdEnums::QSdCount              )
    QP_RW(QSdEnums::QSdRngTypes,                rngType,                        QSdEnums::QSdCudaRNG            )
    QP_RW(QSdEnums::QSdRngTypes,                samplerRngType,                 QSdEnums::QSdRngTypeCount       )
    QP_RW(QSdEnums::QSdPredictionTypes,         prediction,                     QSdEnums::QSdPredictionCount    )
    QP_RW(QSdEnums::QSdLoraApplyModeTypes,      loraApplyMode,                  QSdEnums::QSdLoraAuto           )
    QP_RW(QSdEnums::QSdVaeFormatTypes,          vaeFormat,                      QSdEnums::QSdVaeFormatAuto      )
    //
    QP_PTR_RO(ObjectListModel<QSdEmbedding>,    embeddings                                                      ) // Q_PROPERTY(quint32 embeddingCount READ embeddingCount WRITE setEmbeddingCount NOTIFY embeddingCountChanged FINAL)
    //
    QP_RW(int,                                  numberOfThreads,                0                               )
    QP_RW(int,                                  chromaT5MaskPad,                1                               )
    //
    QP_RW(QString,                              maxVram,                        "0"                             ) // GiB budget for graph-cut segmented param offload (0 = disabled, -1 = auto free VRAM minus 1 GiB)
    //
    QP_RW(bool,                                 enableMmap,                     false                           )
    QP_RW(bool,                                 flashAttn,                      false                           )
    QP_RW(bool,                                 diffusionFlashAttn,             false                           )
    QP_RW(bool,                                 taePreviewOnly,                 false                           )
    QP_RW(bool,                                 diffusionConvDirect,            false                           )
    QP_RW(bool,                                 vaeConvDirect,                  false                           )
    QP_RW(bool,                                 circularX,                      false                           )
    QP_RW(bool,                                 circularY,                      false                           )
    QP_RW(bool,                                 forceSdxlVaeConvScale,          false                           )
    QP_RW(bool,                                 chromaUseDitMask,               true                            )
    QP_RW(bool,                                 chromaUseT5Mask,                false                           )
    QP_RW(bool,                                 streamLayers,                   false                           ) // Enable residency+prefetch streaming on top of --max-vram (no effect without --max-vram)
    QP_RW(bool,                                 qwenImageZero,                  false                           )
    //
    QP_RW(bool,                                 weightsOnCpu,                   false                           )
    QP_RW(bool,                                 clipOnCpu,                      false                           )
    QP_RW(bool,                                 vaeOnCpu,                       false                           )
    QP_RW(bool,                                 controlNetOnCpu,                false                           )
    QML_ELEMENT
    QML_UNCREATABLE("Use QSD.ContextParams...")
public:
    explicit QSdCtxParams(QObject *parent = nullptr);
    ~QSdCtxParams();

    sd_ctx_params_t ctxParams();
    void setCtxParams(sd_ctx_params_t *other);
    void setCtxParams(sd_ctx_params_t other);

    Q_INVOKABLE void addEmbeddings(QString url, QString uid);
    Q_INVOKABLE QString debugString();

private:
    sd_ctx_params_t m_ctxParams{};
    std::vector<sd_embedding_t> m_embeddingVec;

    QByteArray tmp_modelPath                    = "";
    QByteArray tmp_clipLPath                    = "";
    QByteArray tmp_clipGPath                    = "";
    QByteArray tmp_clipVisionPath               = "";
    QByteArray tmp_t5xxlPath                    = "";
    QByteArray tmp_llmPath                      = "";
    QByteArray tmp_llmVisionPath                = "";
    QByteArray tmp_diffusionModelPath           = "";
    QByteArray tmp_highNoiseDiffusionModelPath  = "";
    QByteArray tmp_uncondDiffusionModelPath     = "";
    QByteArray tmp_embeddingsConnectorsPath     = "";
    QByteArray tmp_vaePath                      = "";
    QByteArray tmp_audioVaePath                 = "";
    QByteArray tmp_taesdPath                    = "";
    QByteArray tmp_controlNetPath               = "";
    QByteArray tmp_photoMakerPath               = "";
    //
    QByteArray tmp_tensorTypeRules              = "";
    QByteArray tmp_backend                      = "";
    QByteArray tmp_paramsBackend                = "";
    QByteArray tmp_rpcServers                   = "";

    QByteArray tmp_maxVram                       = "";


};

#endif // QSDCTXPARAMS_H
