#pragma once
#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"
#include "qmlsd_export.h"
class QMLSD_EXPORT QSdLora : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(bool,    isHighNoise, false ) // If true, prefixes tensors with 'model.high_noise_' (architecture-specific routing for models like SDXL).
    QP_RW(float,   multiplier,  0.f   ) // The scaling weight of the LoRA injection (can be negative to invert the learned effect).
    QP_RW(QString, path,        ""    ) // Local file path to the LoRA weights (.safetensors or .ckpt).
    QP_RW(bool,    isEnabled,   false ) // Local UI toggle to bypass this LoRA during inference without clearing its settings (not upstream).
    QML_ELEMENT

public:
    explicit QSdLora(QObject *parent = nullptr);
    ~QSdLora();

    void setLora(sd_lora_t other);  // take in the upstream class and then set this class to its "other"
    sd_lora_t lora(); // return the upstream struct or class from this class
    void resetLora(); // reset to default state

private:
    sd_lora_t m_lora = {false, 0.f, nullptr}; // the upstream object
    QByteArray tmp_path;
};
