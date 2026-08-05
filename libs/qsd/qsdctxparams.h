#pragma once

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

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdCtxParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QString,                              modelPath,                      ""                              ) // Path to the main model file (safetensors/ckpt/gguf).
    QP_RW(QString,                              clipLPath,                      ""                              ) // Path to the CLIP-ViT-L text encoder.
    QP_RW(QString,                              clipGPath,                      ""                              ) // Path to the CLIP-ViT-G text encoder (used in SDXL/SD3).
    QP_RW(QString,                              clipVisionPath,                 ""                              ) // Path to the CLIP Vision encoder (for image conditioning).
    QP_RW(QString,                              t5xxlPath,                      ""                              ) // Path to the T5-XXL text encoder (used in SD3/Flux).
    QP_RW(QString,                              llmPath,                        ""                              ) // Path to the LLM for complex text understanding.
    QP_RW(QString,                              llmVisionPath,                  ""                              ) // Path to the Vision LLM (for image-to-text or multimodal tasks).
    QP_RW(QString,                              diffusionModelPath,             ""                              ) // Path to a standalone UNet or DiT (if separated from the main model).
    QP_RW(QString,                              highNoiseDiffusionModelPath,    ""                              ) // Path to a secondary diffusion model for expert high-noise refinement.
    QP_RW(QString,                              uncondDiffusionModelPath,       ""                              ) // Path to a specific unconditional diffusion model.
    QP_RW(QString,                              embeddingsConnectorsPath,       ""                              ) // Path to the connectors mapping custom embeddings to the model.
    QP_RW(QString,                              vaePath,                        ""                              ) // Path to a standalone Variational Autoencoder to override the baked-in one.
    QP_RW(QString,                              audioVaePath,                   ""                              ) // Path to the audio VAE (for audio-reactive or generation models).
    QP_RW(QString,                              taesdPath,                      ""                              ) // Path to the Tiny Autoencoder for ultra-fast UI previews.
    QP_RW(QString,                              controlNetPath,                 ""                              ) // Path to a ControlNet model for structural conditioning.
    QP_RW(QString,                              photoMakerPath,                 ""                              ) // Path to a PhotoMaker model for consistent character generation.
    //
    QP_RW(QString,                              tensorTypeRules,                ""                              ) // Comma-separated overrides for tensor quantization (e.g., 'blk.0.attn.weight=f16').
    QP_RW(QString,                              backend,                        ""                              ) // Target compute backend for execution (e.g., 'cuda', 'metal', 'vulkan', 'cpu').
    QP_RW(QString,                              paramsBackend,                  ""                              ) // Target compute backend specifically for parameter offloading.
    QP_RW(QString,                              rpcServers,                     ""                              ) // Comma-separated list of RPC server IP:PORT for distributed inference.
    //
    QP_RW(QSdEnums::QSdWeightTypes,             weightType,                     QSdEnums::QSdCount              ) // The quantization format to use when loading weights into RAM/VRAM.
    QP_RW(QSdEnums::QSdRngTypes,                rngType,                        QSdEnums::QSdCudaRNG            ) // The Random Number Generator engine for latent noise initialization.
    QP_RW(QSdEnums::QSdRngTypes,                samplerRngType,                 QSdEnums::QSdRngTypeCount       ) // The RNG engine specifically for sampler step noise.
    QP_RW(QSdEnums::QSdPredictionTypes,         prediction,                     QSdEnums::QSdPredictionCount    ) // The model's prediction objective (epsilon, v-prediction, flow, etc).
    QP_RW(QSdEnums::QSdLoraApplyModeTypes,      loraApplyMode,                  QSdEnums::QSdLoraAuto           ) // When to apply LoRAs (at load time vs runtime inference).
    QP_RW(QSdEnums::QSdVaeFormatTypes,          vaeFormat,                      QSdEnums::QSdVaeFormatAuto      ) // The expected VAE tensor format (Auto, SD3, Flux, etc).
    //
    QP_PTR_RO(ObjectListModel<QSdEmbedding>,    embeddings                                                      ) // MVC List of active textual inversion embeddings.
    //
    QP_RW(int,                                  numberOfThreads,                0                               ) // CPU threads to allocate for processing (0 = auto).
    QP_RW(int,                                  chromaT5MaskPad,                1                               ) // Padding size for T5 masks in Chroma models.
    //
    QP_RW(QString,                              maxVram,                        "0"                             ) // GiB budget for graph-cut segmented param offload (0 = disabled, -1 = auto free VRAM minus 1 GiB).
    //
    QP_RW(bool,                                 enableMmap,                     false                           ) // Use memory mapping to load models instantly (requires weights to stay on disk).
    QP_RW(bool,                                 flashAttn,                      false                           ) // Enable Flash Attention for massive memory savings and speedups.
    QP_RW(bool,                                 diffusionFlashAttn,             false                           ) // Force Flash Attention specifically in the diffusion layers.
    QP_RW(bool,                                 taePreviewOnly,                 false                           ) // Use Tiny Autoencoder exclusively for intermediate step previews, not the final decode.
    QP_RW(bool,                                 diffusionConvDirect,            false                           ) // Use direct convolution mapping in diffusion layers (bypasses some im2col overhead).
    QP_RW(bool,                                 vaeConvDirect,                  false                           ) // Use direct convolution mapping in the VAE layers.
    QP_RW(bool,                                 circularX,                      false                           ) // Enable seamless tiling horizontally.
    QP_RW(bool,                                 circularY,                      false                           ) // Enable seamless tiling vertically.
    QP_RW(bool,                                 forceSdxlVaeConvScale,          false                           ) // Force compatibility scaling for SDXL VAE convolutions.
    QP_RW(bool,                                 chromaUseDitMask,               true                            ) // Enable DiT masking for Chroma models.
    QP_RW(bool,                                 chromaUseT5Mask,                false                           ) // Enable T5 text masking for Chroma models.
    QP_RW(bool,                                 streamLayers,                   false                           ) // Enable residency+prefetch streaming on top of --max-vram (no effect without --max-vram).
    QP_RW(bool,                                 qwenImageZero,                  false                           ) // Enable zero-initialization for Qwen vision embeddings.
    //
    QP_RW(bool,                                 weightsOnCpu,                   false                           ) // Force all model weights to remain in system RAM.
    QP_RW(bool,                                 clipOnCpu,                      false                           ) // Force the CLIP text encoder to execute on the CPU.
    QP_RW(bool,                                 vaeOnCpu,                       false                           ) // Force the VAE decoder to execute on the CPU (saves VRAM for high-res images).
    QP_RW(bool,                                 controlNetOnCpu,                false                           ) // Force the ControlNet conditioning to execute on the CPU.

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
