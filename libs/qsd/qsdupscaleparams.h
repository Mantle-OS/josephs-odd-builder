#pragma once
#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>
#include <property-macros.h>
#include "qsdbaseparam.h"

#include "qmlsd_export.h"

class QMLSD_EXPORT QSdUpscaleParams : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QString, esrganPath,      ""    ) // Path to the ESRGAN upscaler model file.
    QP_RW(bool,    direct,          false ) // Use direct convolution mapping (bypasses some im2col overhead).
    QP_RW(int,     numberOfThreads, 0     ) // CPU threads to allocate for upscaling (0 = auto).
    QP_RW(int,     tileSize,        128   ) // Tile size for tiled upscaling, to bound VRAM usage on large images.
    QP_RW(QString, backend,         ""    ) // Target compute backend for execution (e.g., 'cuda', 'metal', 'vulkan', 'cpu').
    QP_RW(QString, paramsBackend,   ""    ) // Target compute backend specifically for parameter offloading.
    QML_ELEMENT
    QML_UNCREATABLE("Use QSd.UpscaleParams")
public:
    explicit QSdUpscaleParams(QObject *parent = nullptr) :
        QSdBaseParam{parent}{}
};