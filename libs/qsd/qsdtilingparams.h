#ifndef QSDTILINGPARAMS_H
#define QSDTILINGPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <property-macros.h>

#include "qsdbaseparam.h"

class QSdTilingParams : public QSdBaseParam
{
    Q_OBJECT

    QP_RW(bool,     isEnabled,          false       )
    QP_RW(bool,     temporalTiling,     false       )
    QP_RW(int,      tileSizeX,          0           )
    QP_RW(int,      tileSizeY,          0           )
    QP_RW(float,    targetOverlap,      0.5f        )
    QP_RW(float,    relSizeX,           0.0f        )
    QP_RW(float,    relSizeY,           0.0f        )
    QP_RW(QString,  extraTilingArgs,    QString{}   )

    QML_ELEMENT
public:
    explicit QSdTilingParams(QObject *parent = nullptr);
    ~QSdTilingParams();

    sd_tiling_params_t tilingParams();
    void setTilingParams(sd_tiling_params_t other);
    void resetTilingParams();

private:
    sd_tiling_params_t  m_tilingParams{false, false, 0, 0, 0.5f, 0.0f, 0.0f, nullptr};
    QByteArray tmp_extraTilingArgs;
};

#endif // QSDTILINGPARAMS_H
