// qsdvidgenparams.h
#pragma once
#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <objectmodel.h>
#include <pointer-macros.h>

#include "qsdbaseparam.h"
#include "qsdcacheparams.h"
#include "qsdhiresparams.h"
#include "qsdimage.h"
#include "qsdlora.h"
#include "qsdsampleparams.h"
#include "qsdtilingparams.h"

#include "qmlsd_export.h"

class QMLSD_EXPORT QSdVidGenParams : public QSdBaseParam
{
    Q_OBJECT
    QP_PTR_RO(ObjectListModel<QSdLora>,   loras                                  ) // MVC list of active LoRAs applied to this generation.
    QP_RW(QString,                        prompt,                 ""             ) // Positive prompt describing the desired video.
    QP_RW(QString,                        negativePrompt,         ""             ) // Negative prompt describing what to avoid in the video.
    QP_RW(int,                            clipSkip,               -1             ) // Number of final CLIP layers to skip (-1 = model default).
    QP_PTR_RW(QSdImage,                   initImage                              ) // First-frame source image for image-to-video.
    QP_PTR_RW(QSdImage,                   endImage                               ) // Last-frame target image, for start/end-conditioned video generation.
    QP_PTR_RO(ObjectListModel<QSdImage>,  controlFrames                          ) // MVC list of per-frame ControlNet/VACE conditioning images.
    QP_RW(int,                            width,                  512            ) // Output video frame width, in pixels.
    QP_RW(int,                            height,                 512            ) // Output video frame height, in pixels.
    QP_PTR_RW(QSdSampleParams,            sampleParams                           ) // Sampler configuration for the low-noise (primary) pass.
    QP_PTR_RW(QSdSampleParams,            highNoiseSampleParams                  ) // Sampler configuration for the high-noise (MoE expert) pass.
    QP_RW(float,                          moeBoundary,            0.875f         ) // Timestep boundary separating the high-noise and low-noise MoE experts.
    QP_RW(float,                          strength,               0.75f          ) // img2vid denoising strength (0 = keep init image, 1 = fully regenerate).
    QP_RW(qint64,                         seed,                   -1             ) // RNG seed for reproducible generations (-1 = random each run).
    QP_RW(int,                            videoFrames,            6              ) // Total number of frames to generate.
    QP_RW(int,                            fps,                    24             ) // Output video frame rate.
    QP_RW(float,                          vaceStrength,           1.0f           ) // VACE conditioning strength (0 = ignored, 1 = full influence).
    QP_PTR_RO(QSdTilingParams,            vaeTilingParams                        ) // VAE tiling parameters, for low-VRAM high-resolution decode.
    QP_PTR_RO(QSdCacheParams,             cache                                  ) // Step-caching parameters for accelerated inference.
    QP_PTR_RO(QSdHiResParams,             hires                                  ) // Hi-res fix parameters, for two-pass upscaled generation.


    QML_ELEMENT
    QML_UNCREATABLE("Please Use QSD.VideoGenerationParams")

public:
    explicit QSdVidGenParams(QObject *parent = nullptr);
    ~QSdVidGenParams();

    sd_vid_gen_params_t *vidGenParams();
    void setVidGenParams(sd_vid_gen_params_t *other);
    void resetVidGenParams();

private:
    sd_vid_gen_params_t *m_vidGenParams = nullptr;
    std::vector<sd_lora_t> m_loraVec;
    std::vector<sd_image_t> m_proxyControlFrames;

    QByteArray tmp_prompt;
    QByteArray tmp_negativePrompt;
};