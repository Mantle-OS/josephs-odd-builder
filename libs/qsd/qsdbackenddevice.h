#ifndef QSDBACKENDDEVICE_H
#define QSDBACKENDDEVICE_H

#include "qsdbaseparam.h"
#include "qsdenums.h"

class QSdBackendDevice : public QSdBaseParam
{
    Q_OBJECT
    QML_ELEMENT

    // Properties that expose structural assignments cleanly to QML and Serialization loops
    QP_RW(int,          moduleType,         static_cast<int>(QSdEnums::Diffusion)       )
    QP_RW(QString,      runtimeTarget,      "cuda"                                      ) // "cuda", "cpu", "vulkan", "rpc"
    QP_RW(QString,      paramsTarget,       "cuda"                                      ) // Weights matrix allocation domain
    QP_RW(int,          cpuThreadLimit,         4                                       ) // Tied directly to sd_backend_cpu_set_n_threads

public:
    explicit QSdBackendDevice(QObject *parent = nullptr) : QSdBaseParam(parent) {}
    ~QSdBackendDevice() = default;
};

#endif // QSDBACKENDDEVICE_H