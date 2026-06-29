#ifndef QSDUPSCALEPARAMS_H
#define QSDUPSCALEPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

class QSdUpscaleParams : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QSdUpscaleParams(QObject *parent = nullptr);

Q_SIGNALS:


private:
    upscaler_ctx_t* m_upscalerCtx = nullptr;

};

#endif // QSDUPSCALEPARAMS_H
