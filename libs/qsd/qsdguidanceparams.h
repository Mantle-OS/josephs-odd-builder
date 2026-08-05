#pragma once
#include <QObject>
#include <QtMath>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>
#include <property-macros.h>

#include "qsdbaseparam.h"
#include "qsdslgparams.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdGuidanceParams : public QSdBaseParam
{
    Q_OBJECT 
    QP_RW(float,            txtCfg,            7.0f    ) // Text classifier-free guidance scale (higher = strictly follows prompt).
    QP_RW(float,            imgCfg,            qInf()  ) // Image classifier-free guidance scale (using qInf() to dodge macro bugs).
    QP_RW(float,            distilledGuidance, 3.5f    ) // Specialized guidance scale for distilled models like Flux.
    QP_PTR_RO(QSdSlgParams, slg                        ) // Skip Layer Guidance parameter block.

    QML_ELEMENT

public:
    explicit QSdGuidanceParams(QObject *parent = nullptr);
    ~QSdGuidanceParams();

    sd_guidance_params_t guidanceParams();
    void setGuidanceParams(sd_guidance_params_t other);
    void resetGuidanceParams();

private:
    sd_guidance_params_t m_guidanceParams{};
};

