#include "qsdaudio.h"

QSdAudio::QSdAudio(QObject *parent) :
    QSdBaseParam{parent}
{
    resetAudio();
}

QSdAudio::~QSdAudio()
{
    resetAudio();
    if(m_data) {
        delete m_data;
        m_data = nullptr;
    }
}

sd_audio_t QSdAudio::audio()
{
    sd_audio_t ret;
    ret.sample_rate     = m_sampleRate;
    ret.channels        = m_channels;
    ret.sample_count    = m_sampleCount;
    ret.data            = m_data;
    return ret;
}

void QSdAudio::setAudio(sd_audio_t other)
{
    set_sampleRate(other.sample_rate);
    set_channels(other.channels);
    set_sampleCount(other.sample_count);
    setData(other.data);
    m_audio = other;
}

void QSdAudio::resetAudio()
{
    m_audio = {0, 0, 0, nullptr};
    if(m_data){
        m_data = nullptr;
        Q_EMIT dataChanged();
    }
}

float *QSdAudio::data() const
{
    return m_data;
}

void QSdAudio::setData(float *newData)
{
    if (m_data == newData) return;
    m_data = newData;
    Q_EMIT dataChanged();
}
