#include "qsdlora.h"
#include <qaiutils.h>

QSdLora::QSdLora(QObject *parent) :
    QSdBaseParam{parent}
{
    resetLora();
}

QSdLora::~QSdLora()
{
}

void QSdLora::setLora(sd_lora_t other)
{
    set_isHighNoise(other.is_high_noise);
    set_multiplier(other.multiplier);
    set_path(other.path ? QString::fromUtf8(other.path, strnlen(other.path, 2048)) : QString{});

    m_lora = other;
}

sd_lora_t QSdLora::lora()
{
    sd_lora_t ret = {false, 0.0f, nullptr};
    if(!m_path.isEmpty() && QAiUtils::fileExists(m_path) && m_isEnabled){
        ret.is_high_noise   = m_isHighNoise;
        ret.multiplier      = m_multiplier;

        tmp_path = m_path.toLocal8Bit();
        ret.path = tmp_path.constData();
    }
    m_lora = ret;
    return ret;

}

void QSdLora::resetLora()
{
    m_lora = {false, 0.0f, nullptr};
    set_isEnabled(false);
    setLora(m_lora);
}
