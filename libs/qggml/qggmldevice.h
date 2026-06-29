#ifndef QGGMLDEVICE_H
#define QGGMLDEVICE_H

#include <QObject>
#include <QQmlEngine>
#include <property-macros.h>
#include <pointer-macros.h>

class QGgmlDevice : public QObject
{
    Q_OBJECT
    QP_RO(qsizetype, uid, 0) // device_id
    QP_RO(QString, name, "")
    QP_RO(QString, description, "")
    QP_RO(qsizetype, usedMemory, 0)
    QP_RO(qsizetype, totalMemory, 0)

    QML_ELEMENT
public:
    explicit QGgmlDevice(QObject *parent = nullptr);


    enum QGgmlDeviceType {
        CPU,    // CPU device using system memory
        GPU,    // GPU device using dedicated memory
        IGPU,   // integrated GPU device using host memory
        ACCEL,  // accelerator devices intended to be used together with the CPU backend (e.g. BLAS or AMX)
        META    // "meta" device wrapping multiple other devices for tensor parallelism
    };
    Q_ENUM(QGgmlDeviceType)



signals:
};

#endif // QGGMLDEVICE_H
