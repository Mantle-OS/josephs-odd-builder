#ifndef QSDCACHEPARAMS_H
#define QSDCACHEPARAMS_H

#include <QObject>
#include <QQmlEngine>
#include <stable-diffusion.h>

#include "qsdenums.h"
#include "qsdbaseparam.h"

class QSdCacheParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QSdEnums::QSdCacheModeTypes,      mode,                       QSdEnums::QSdCacheDisabled  )
    QP_RW(float,                            reuseThreshold,             INFINITY                    )
    QP_RW(float,                            startPercent,               0.15f                       )
    QP_RW(float,                            endPercent,                 0.95f                       )
    QP_RW(float,                            errorDecayRate,             1.0f                        )
    QP_RW(bool,                             useRelativeThreshold,       true                        )
    QP_RW(bool,                             resetErrorOnCompute,        true                        )
    QP_RW(int,                              fnComputeBlocks,            8                           )
    QP_RW(int,                              bnComputeBlocks,            0                           )
    QP_RW(float,                            residualDiffThreshold,      0.08f                       )
    QP_RW(int,                              maxWarmupSteps,             8                           )
    QP_RW(int,                              maxCachedSteps,             -1                          )
    QP_RW(int,                              maxContinuousCachedSteps,   -1                          )
    QP_RW(int,                              taylorseerNDerivatives,     1                           )
    QP_RW(int,                              taylorseerSkipInterval,     1                           )
    QP_RW(QString,                          scmMask,                    ""                          )
    QP_RW(bool,                             scmPolicyDynamic,           true                        )
    QP_RW(float,                            spectrumW,                  0.40f                       )
    QP_RW(int,                              spectrumM,                  3                           )
    QP_RW(float,                            spectrumLam,                1.0f                        )
    QP_RW(int,                              spectrumWindowSize,         2                           )
    QP_RW(float,                            spectrumFlexWindow,         0.50f                       )
    QP_RW(int,                              spectrumWarmupSteps,        4                           )
    QP_RW(float,                            spectrumStopPercent,        0.9f                        )

    QML_ELEMENT
public:
    explicit QSdCacheParams(QObject *parent = nullptr);
    ~QSdCacheParams();

    sd_cache_params_t cacheParams();
    void setCacheParams(sd_cache_params_t other);
    void resetCacheParams();

private:
    sd_cache_params_t m_cacheParams = {};
    QByteArray tmp_scmMask;
};

#endif // QSDCACHEPARAMS_H
