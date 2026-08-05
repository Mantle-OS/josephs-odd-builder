#include "qsdhiresparams.h"

QSdHiResParams::QSdHiResParams(QObject *parent) :
    QSdBaseParam{parent}
{
    resetHiresParams(); // Properly initialize the struct and Qt properties!
}

QSdHiResParams::~QSdHiResParams()
{
}

QList<float> QSdHiResParams::customSigmas() const
{
    return m_customSigmas;
}

void QSdHiResParams::appendCustomSigma(float sigmas)
{
    m_customSigmas.append(sigmas);
    Q_EMIT customSigmasChanged();
}

void QSdHiResParams::prependCustomSigma(float sigmas)
{
    m_customSigmas.prepend(sigmas);
    Q_EMIT customSigmasChanged();
}

void QSdHiResParams::clearCustomSigmas()
{
    m_customSigmas.clear();
    m_proxySigmas.clear();
    Q_EMIT customSigmasChanged();
}

sd_hires_params_t QSdHiResParams::hiresParams()
{
    sd_hires_params_t ret{};
    ret.enabled  = m_isEnabled;
    ret.upscaler = QSdEnums::sdHiResUpscalerType(m_upscaler);

    if(!m_modelPath.isEmpty() && QAiUtils::fileExists(m_modelPath) && m_isEnabled){
        tmp_modelPath = m_modelPath.toUtf8();
        ret.model_path = tmp_modelPath.constData();
    } else {
        ret.model_path = nullptr;
    }

    ret.scale              = m_scale;
    ret.target_width       = m_targetWidth;
    ret.target_height      = m_targetHeight;
    ret.steps              = m_steps;
    ret.denoising_strength = m_denoisingStrength;
    ret.upscale_tile_size  = m_upscaleTileSize;

    if(!m_customSigmas.isEmpty()){
        m_proxySigmas.clear();
        m_proxySigmas.reserve(m_customSigmas.size()); // Safe reserve!

        for(float i : m_customSigmas.toList())
            m_proxySigmas.push_back(i);
        ret.custom_sigmas       = m_proxySigmas.data();
        ret.custom_sigmas_count = m_proxySigmas.size();
    }else{
        ret.custom_sigmas       = nullptr;
        ret.custom_sigmas_count = 0;
    }

    m_hiresParams = ret;
    return ret;
}

void QSdHiResParams::setHiresParams(const sd_hires_params_t &other)
{
    set_isEnabled(other.enabled);
    set_upscaler(QSdEnums::qsdHiResUpscalerType(other.upscaler));

    set_modelPath(other.model_path ? QString::fromUtf8(other.model_path, strnlen(other.model_path, 2048)) : QString{});

    set_scale(other.scale);
    set_targetWidth(other.target_width);
    set_targetHeight(other.target_height);
    set_steps(other.steps);
    set_denoisingStrength(other.denoising_strength);
    set_upscaleTileSize(other.upscale_tile_size);

    clearCustomSigmas();
    if (other.custom_sigmas && other.custom_sigmas_count > 0) {
        for (size_t i = 0; i < other.custom_sigmas_count; ++i)
            m_customSigmas.append(other.custom_sigmas[i]);
        Q_EMIT customSigmasChanged(); // Fire again so UI updates with new values
    }

    m_hiresParams = other;
}

void QSdHiResParams::resetHiresParams()
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