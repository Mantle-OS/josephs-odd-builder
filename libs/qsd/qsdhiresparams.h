#pragma once

#include <vector>

#include <QObject>
#include <QQmlEngine>


#include <stable-diffusion.h>

#include <property-macros.h>
#include <qaiutils.h>

#include "qsdbaseparam.h"
#include "qsdenums.h"

// note not fully tested
class QSdHiResParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(bool,                                 isEnabled,          false                               )
    QP_RW(QSdEnums::QSdHiResUpscalerTypes,      upscaler,           QSdEnums::QSdHiResUpscalerLatent    )
    QP_RW(QString,                              modelPath,          ""                                  )
    QP_RW(float,                                scale,              2.f                                 )
    QP_RW(int,                                  targetWidth,        0                                   )
    QP_RW(int,                                  targetHeight,       0                                   )
    QP_RW(int,                                  steps,              0                                   )
    QP_RW(float,                                denoisingStrength,  0.7f                                )
    QP_RW(int,                                  upscaleTileSize,    128                                 )
    // QP_RW(int customSigmasCount)
    Q_PROPERTY(QList<float> customSigmas READ customSigmas NOTIFY customSigmasChanged FINAL)
    QML_ELEMENT

public:
    explicit QSdHiResParams(QObject *parent = nullptr);
    ~QSdHiResParams();

    QList<float> customSigmas() const
    {
        return m_customSigmas;
    }

    Q_INVOKABLE void appendCustomSigma(float sigmas)
    {
        m_customSigmas.append(sigmas);
        Q_EMIT customSigmasChanged();
    }

    Q_INVOKABLE void prependCustomSigma(float sigmas)
    {
        m_customSigmas.prepend(sigmas);
        Q_EMIT customSigmasChanged();
    }

    Q_INVOKABLE void clearCustomSigmas()
    {
        m_customSigmas.clear();
        m_proxySigmas.clear();
        Q_EMIT customSigmasChanged();
    }

    sd_hires_params_t hiresParams()
    {
        sd_hires_params_t ret;
        ret.enabled     = m_isEnabled;
        ret.upscaler    = QSdEnums::sdHiResUpscalerType(m_upscaler);
        if(!m_modelPath.isEmpty() && QAiUtils::fileExists(m_modelPath)){
            tmp_modelPath = m_modelPath.toUtf8();
            ret.model_path = tmp_modelPath.constData();
        }else{
            ret.model_path = nullptr;
        }
        ret.scale = m_scale;
        ret.target_width = m_targetWidth;
        ret.target_height = m_targetHeight;
        ret.steps = m_steps;
        ret.denoising_strength = m_denoisingStrength;
        ret.upscale_tile_size = m_upscaleTileSize;
        if(!m_customSigmas.isEmpty()){

            m_proxySigmas.clear();
            m_proxySigmas.resize(m_customSigmas.size());

            for(float i : m_customSigmas.toList()){
                m_proxySigmas.push_back(i);
            }
            ret.custom_sigmas       = m_proxySigmas.data();
            ret.custom_sigmas_count = m_proxySigmas.size();
        }else{
            ret.custom_sigmas       = nullptr;
            ret.custom_sigmas_count = 0;
        }
        return ret;
    }

    void setHiresParams(const sd_hires_params_t &other){
        set_isEnabled(other.enabled);
        set_upscaler( QSdEnums::qsdHiResUpscalerType(other.upscaler));
        if(other.model_path){
            set_modelPath(QString::fromLatin1(other.model_path));
        }else{
            set_modelPath("");
        }
        set_scale(other.scale);
        set_targetWidth(other.target_width);
        set_targetHeight(other.target_height);
        set_steps(other.steps);
        set_denoisingStrength(other.denoising_strength);
        set_upscaleTileSize(other.upscale_tile_size);

        clearCustomSigmas();
        if (other.custom_sigmas && other.custom_sigmas_count > 0) {
            for (size_t i = 0; i < other.custom_sigmas_count; ++i) {
                m_customSigmas.append(other.custom_sigmas[i]);
            }
        }
    }

    void resetHiresParams()
    {
        m_hiresParams = {
            false,
            SD_HIRES_UPSCALER_LATENT,
            nullptr,
            2.0f,
            0, 0, 0,
            0.7f,
            128,
            nullptr,
            0
        };
        setHiresParams(m_hiresParams);
    }

Q_SIGNALS:
    void customSigmasChanged();

private:
    sd_hires_params_t m_hiresParams = {
        false,
        SD_HIRES_UPSCALER_LATENT,
        nullptr,
        2.0f,
        0, 0, 0,
        0.7f,
        128,
        nullptr,
        0
    };
    QList<float> m_customSigmas;
    std::vector<float> m_proxySigmas;
    QByteArray tmp_modelPath;
};

