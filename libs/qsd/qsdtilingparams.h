#pragma once

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <property-macros.h>

#include "qsdbaseparam.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdTilingParams : public QSdBaseParam
{
    Q_OBJECT

    QP_RW(bool,     isEnabled,          false       ) // Enables VAE/UNet tiling to generate huge images without blowing up VRAM.
    QP_RW(bool,     temporalTiling,     false       ) // Enables tiling across the time dimension (frames) for video generation models.
    QP_RW(int,      tileSizeX,          0           ) // Absolute width of each individual tile in pixels (0 for auto/default).
    QP_RW(int,      tileSizeY,          0           ) // Absolute height of each individual tile in pixels (0 for auto/default).
    QP_RW(float,    targetOverlap,      0.5f        ) // The overlap ratio between tiles to seamlessly blend edges (0.5 = 50% overlap).
    QP_RW(float,    relSizeX,           0.0f        ) // Relative width multiplier for the tiles (alternative to absolute tileSizeX).
    QP_RW(float,    relSizeY,           0.0f        ) // Relative height multiplier for the tiles (alternative to absolute tileSizeY).
    QP_RW(QString,  extraTilingArgs,    QString{}   ) // Pass-through string for backend-specific tweaks (e.g. "temporal_tile_frames=24").

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
