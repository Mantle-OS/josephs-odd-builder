#pragma once
#include <memory.h>

#include <QObject>
#include <QtMath>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>

#include "qsdguidanceparams.h"
#include "qsdenums.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdSampleParams : public QSdBaseParam
{
    Q_OBJECT    
    QP_RW(QSdEnums::QSdSchedulerTypes,  scheduler,          QSdEnums::QSdSchedulerCount ) // The noise scheduler algorithm (e.g., Euler, Karras).
    QP_RW(QSdEnums::QSdSampleTypes,     sampleMethod,       QSdEnums::QSdSampleCount    ) // The sampling method (e.g., Euler A, DPM++ 2M).
    QP_RW(int,                          sampleSteps,        20                          ) // Number of denoising steps (higher = more detail, slower generation).
    QP_RW(float,                        eta,                qInf()                      ) // Noise multiplier for ancestral samplers (usually 1.0 or Inf).
    QP_RW(int,                          shiftedTimestep,    0                           ) // Timestep shift (specific to certain experimental schedules).
    QP_RW(float,                        flowShift,          qInf()                      ) // Flow matching shift value (critical for Flux/SD3 scaling).
    QP_RW(QString,                      extraSampleArgs,    ""                          ) // Optional extra key=value arguments for custom samplers.
    QP_PTR_RO(QSdGuidanceParams,        guidance                                        ) // Nested CFG and SLG parameters object.
    Q_PROPERTY(QList<float>             customSigmas        READ customSigmas           NOTIFY customSigmasChanged FINAL   )// steps on a custom sigma Explicit user-defined noise schedule (overrides sampler defaults).
    QML_ELEMENT

public:
    explicit QSdSampleParams(QObject *parent = nullptr);
    ~QSdSampleParams();

    sd_sample_params_t sampleParams();
    void setSampleParams(sd_sample_params_t other);
    void resetSampleParams();

    QList<float> customSigmas() const;
    Q_INVOKABLE void appendCustomSigma(float sigmas);
    Q_INVOKABLE void prependCustomSigma(float sigmas);
    Q_INVOKABLE void clearCustomSigmas();
    Q_INVOKABLE QString debugString();

Q_SIGNALS:
    void customSigmasChanged();

private:
    sd_sample_params_t m_sampleParams{};
    QByteArray tmp_extraSampleArgs;
    QList<float> m_customSigmas;
    std::vector<float> m_proxySigmas;
};
