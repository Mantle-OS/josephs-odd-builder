#pragma once

#include <vector>

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <property-macros.h>
#include <qaiutils.h>

#include "qsdbaseparam.h"
#include "qsdenums.h"
#include "qmlsd_export.h"

class QMLSD_EXPORT QSdHiResParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(bool,                            isEnabled,         false                            ) // Master toggle for high-res fix.
    QP_RW(QSdEnums::QSdHiResUpscalerTypes, upscaler,          QSdEnums::QSdHiResUpscalerLatent ) // The upscaler algorithm to use (e.g., Latent, ESRGAN).
    QP_RW(QString,                         modelPath,         ""                               ) // Path to the external upscale model weights.
    QP_RW(float,                           scale,             2.f                              ) // Multiplication factor for resolution scaling (e.g., 2.0x).
    QP_RW(int,                             targetWidth,       0                                ) // Explicit target width (overrides 'scale' if non-zero).
    QP_RW(int,                             targetHeight,      0                                ) // Explicit target height (overrides 'scale' if non-zero).
    QP_RW(int,                             steps,             0                                ) // Second-pass steps (0 inherits steps from base generation).
    QP_RW(float,                           denoisingStrength, 0.7f                             ) // How much to alter the base image during the upscale pass.
    QP_RW(int,                             upscaleTileSize,   128                              ) // Tiling size for VRAM preservation.

    Q_PROPERTY(QList<float> customSigmas READ customSigmas NOTIFY customSigmasChanged FINAL)

    QML_ELEMENT

public:
    explicit QSdHiResParams(QObject *parent = nullptr);
    ~QSdHiResParams();

    QList<float> customSigmas() const;
    Q_INVOKABLE void appendCustomSigma(float sigmas);
    Q_INVOKABLE void prependCustomSigma(float sigmas);
    Q_INVOKABLE void clearCustomSigmas();

    sd_hires_params_t hiresParams();
    void setHiresParams(const sd_hires_params_t &other);
    void resetHiresParams();

Q_SIGNALS:
    void customSigmasChanged();

private:
    sd_hires_params_t m_hiresParams = {};
    QList<float> m_customSigmas;
    std::vector<float> m_proxySigmas;
    QByteArray tmp_modelPath;
};