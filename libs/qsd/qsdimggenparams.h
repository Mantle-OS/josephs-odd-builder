#pragma once
#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <objectmodel.h>
#include <pointer-macros.h>

#include "qsdcacheparams.h"
#include "qsdhiresparams.h"
#include "qsdlora.h"
#include "qsdimage.h"
#include "qsdpulidparams.h"
#include "qsdsampleparams.h"
#include "qsdpmparams.h"
#include "qsdtilingparams.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdImgGenParams : public QSdBaseParam
{
    // Q_PROPERTY(quint32 loraCount)
    Q_OBJECT
    QP_PTR_RO(ObjectListModel<QSdLora>,     loras                           ) // MVC list of active LoRAs applied to this generation.
    QP_RW(QString,                          prompt,                 ""      ) // Positive prompt describing the desired image.
    QP_RW(QString,                          negativePrompt,         ""      ) // Negative prompt describing what to avoid in the image.
    QP_RW(int,                              clipSkip,               -1      ) // Number of final CLIP layers to skip (-1 = model default).
    QP_PTR_RW(QSdImage,                     initImage                       ) // Source image for img2img; null for pure txt2img.
    QP_PTR_RO(ObjectListModel<QSdImage>,    refImages                      ) // MVC list of reference images for image-conditioned generation.
    QP_RW(int,                              refImagesCount,         0       ) // Number of reference images currently supplied.
    QP_RW(bool,                             autoResizeRefImage,     false   ) // If true, automatically resizes the reference image to match the target output dimensions.
    QP_RW(bool,                             increaseRefIndex,       false   ) // If true, advances to the next reference image slot/index on each use.
    QP_PTR_RW(QSdImage,                     maskImage                       ) // Inpainting mask; white regions are regenerated, black regions are preserved.
    QP_RW(int,                              imgWidth,               512     ) // Output image width, in pixels.
    QP_RW(int,                              imgHeight,              512     ) // Output image height, in pixels.
    QP_PTR_RW(QSdSampleParams,              sampleParams                    ) // Sampler configuration (steps, CFG scale, sampler/scheduler choice, etc).
    QP_RW(float,                            strength,               0.75f   ) // img2img denoising strength (0 = keep init image, 1 = fully regenerate).
    QP_RW(qint64,                           seed,                   -1      ) // RNG seed for reproducible generations (-1 = random each run).
    QP_RW(int,                              batchCount,             1       ) // Number of images to generate in this batch.
    QP_PTR_RW(QSdImage,                     controlImage                    ) // ControlNet conditioning image.
    QP_RW(float,                            controlStrength,        0.9f    ) // ControlNet conditioning strength (0 = ignored, 1 = full influence).
    QP_PTR_RO(QSdPmParams,                  pmParams                        ) // PhotoMaker parameters, for consistent-character generation.
    QP_PTR_RO(QSdPulidParams,               pulidParams                     ) // PuLID identity-conditioning parameters. Enabled separately by loading a PuLID weights model on the context;
    QP_PTR_RO(QSdTilingParams,              vaeTilingParams                 ) // VAE tiling parameters, for low-VRAM high-resolution decode.
    QP_PTR_RO(QSdCacheParams,               cache                           ) // Step-caching parameters for accelerated inference.
    QP_PTR_RO(QSdHiResParams,               hires                           ) // Hi-res fix parameters, for two-pass upscaled generation.
    QML_ELEMENT
    QML_UNCREATABLE("Please Use QSD.GenerationParams")

public:
    explicit QSdImgGenParams(QObject *parent = nullptr);
    ~QSdImgGenParams();

    Q_INVOKABLE QString debugString();

    sd_img_gen_params_t imgGenParms();
    void setImgGenParms(sd_img_gen_params_t other);
    void resetImgGenParms();

private:
    sd_img_gen_params_t m_imgGenParms{};
    std::vector<sd_lora_t> m_loraVec;

    QByteArray tmp_prompt;
    QByteArray tmp_negativePrompt;

    std::vector<sd_image_t> m_proxyRefImages;
};
