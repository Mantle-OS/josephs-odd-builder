#pragma once
#include <QObject>
#include <QtMath>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <real_type.h>

#include "qsdenums.h"
#include "qsdbaseparam.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdCacheParams : public QSdBaseParam
{
    Q_OBJECT    
    QP_RW(QSdEnums::QSdCacheModeTypes, mode,                     QSdEnums::QSdCacheDisabled ) // The caching algorithm to use (e.g., Disabled, TeaCache, TaylorSeer).
    QP_RW(float,                       reuseThreshold,           job::core::safeInfinity() ) // qInf()                     ) // Threshold for tensor reuse; higher means more speed but lower quality.
    QP_RW(float,                       startPercent,             0.15f                      ) // Timestep fraction (0.0 to 1.0) to begin applying the cache.
    QP_RW(float,                       endPercent,               0.95f                      ) // Timestep fraction (0.0 to 1.0) to stop applying the cache.
    QP_RW(float,                       errorDecayRate,           1.0f                       ) // Rate at which accumulated caching error decays.
    QP_RW(bool,                        useRelativeThreshold,     true                       ) // If true, scales the reuse threshold relative to the initial feature diff.
    QP_RW(bool,                        resetErrorOnCompute,      true                       ) // If true, wipes accumulated error after a full non-cached compute step.
    QP_RW(int,                         fnComputeBlocks,          8                          ) // Number of First-N blocks to compute unconditionally.
    QP_RW(int,                         bnComputeBlocks,          0                          ) // Number of Block-N blocks to compute unconditionally.
    QP_RW(float,                       residualDiffThreshold,    0.08f                      ) // Allowed residual difference before forcing a layer recompute.
    QP_RW(int,                         maxWarmupSteps,           8                          ) // Initial steps run strictly without caching to stabilize features.
    QP_RW(int,                         maxCachedSteps,           -1                         ) // Absolute limit on cached steps per generation (-1 for unlimited).
    QP_RW(int,                         maxContinuousCachedSteps, -1                         ) // Limit on *consecutive* cached steps to prevent severe drift (-1 for unlimited).
    QP_RW(int,                         taylorseerNDerivatives,   1                          ) // Number of Taylor expansion derivatives used for TaylorSeer predictive caching.
    QP_RW(int,                         taylorseerSkipInterval,   1                          ) // Step interval between TaylorSeer full recomputations.
    QP_RW(QString,                     scmMask,                  ""                         ) // String defining the Spatial Context Masking structure.
    QP_RW(bool,                        scmPolicyDynamic,         true                       ) // If true, dynamically updates the Spatial Context Masking policy over time.
    QP_RW(float,                       spectrumW,                0.40f                      ) // Spectral caching weight parameter.
    QP_RW(int,                         spectrumM,                3                          ) // Spectral caching momentum/mode count.
    QP_RW(float,                       spectrumLam,              1.0f                       ) // Spectral caching lambda factor.
    QP_RW(int,                         spectrumWindowSize,       2                          ) // Number of previous steps retained in the spectral window.
    QP_RW(float,                       spectrumFlexWindow,       0.50f                      ) // Flexibility threshold for adjusting the spectral window size.
    QP_RW(int,                         spectrumWarmupSteps,      4                          ) // Initial steps before spectral caching activates.
    QP_RW(float,                       spectrumStopPercent,      0.9f                       ) // Timestep fraction (0.0 to 1.0) to disable spectral caching near the end.
    QML_ELEMENT
public:
    explicit QSdCacheParams(QObject *parent = nullptr);
    ~QSdCacheParams();

    sd_cache_params_t cacheParams();
    void setCacheParams(sd_cache_params_t other);
    void resetCacheParams();

private:
    sd_cache_params_t m_cacheParams = {}; // cache of the cache of the .....cache
    QByteArray tmp_scmMask;
};

