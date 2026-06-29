#ifndef QSDGUIDANCEPARAMS_H
#define QSDGUIDANCEPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>
#include <property-macros.h>

#include "qsdbaseparam.h"
#include "qsdslgparams.h"

class QSdGuidanceParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(float, txtCfg,            8.0f        )
    QP_RW(float, imgCfg,            INFINITY    )
    QP_RW(float, distilledGuidance, 3.5f        )
    QP_PTR_RO(QSdSlgParams, slg)

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

#endif // QSDGUIDANCEPARAMS_H

