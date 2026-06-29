#ifndef QSDLORA_H
#define QSDLORA_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"

class QSdLora : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(bool,     isHighNoise,    false   )
    QP_RW(float,    multiplier,     0.f     )
    QP_RW(QString,  path,           ""      )
    QP_RW(bool,     isEnabled,      false   )
    QML_ELEMENT

public:
    explicit QSdLora(QObject *parent = nullptr);
    ~QSdLora();

    void setLora(sd_lora_t other);
    sd_lora_t lora();
    void resetLora();

private:
    sd_lora_t m_lora = {false, 0.f, nullptr};
    QByteArray tmp_path;


};

#endif // QSDLORA_H
