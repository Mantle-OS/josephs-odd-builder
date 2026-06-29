#ifndef QSDSAMPLEPARAMS_H
#define QSDSAMPLEPARAMS_H

#include <memory.h>

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>

#include "qsdguidanceparams.h"
#include "qsdenums.h"

class QSdSampleParams : public QSdBaseParam
{
    Q_OBJECT

    QP_RW(QSdEnums::QSdSchedulerTypes,  scheduler,          QSdEnums::QSdSchedulerCount       )
    QP_RW(QSdEnums::QSdSampleTypes,     sampleMethod,       QSdEnums::QSdSampleCount          )
    QP_RW(int,                          sampleSteps,        20                                )
    QP_RW(float,                        eta,                INFINITY                          )
    QP_RW(int,                          shiftedTimestep,    0                                 )
    QP_RW(float,                        flowShift,          INFINITY                          )
    QP_RW(QString,                      extraSampleArgs,    ""                                )

    QP_PTR_RO(QSdGuidanceParams, guidance)
    Q_PROPERTY(QList<float> customSigmas READ customSigmas NOTIFY customSigmasChanged FINAL   )
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

#endif // QSDSAMPLEPARAMS_H
