#pragma once

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"
#include "qmlsd_export.h"


// NOT TESTED YET
class QMLSD_EXPORT QSdAudio : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(quint32,  sampleRate,      0) // The audio sample rate in Hz (e.g., 16000, 44100).
    QP_RW(quint32,  channels,        0) // The number of audio channels (e.g., 1 for mono, 2 for stereo).
    QP_RW(quint64,  sampleCount,     0) // The total number of float samples in the data buffer.

    // Q_PROPERTY(float *data READ data NOTIFY dataChanged FINAL) // ## not really for QML land

    QML_ELEMENT
public:
    explicit QSdAudio(QObject *parent = nullptr);
    ~QSdAudio();

    float *data() const;
    void setData(float *newData);

    sd_audio_t audio();
    void setAudio(sd_audio_t newAudio);
    void resetAudio();

Q_SIGNALS:
    void dataChanged();

private:
    sd_audio_t m_audio{0, 0, 0, nullptr};
    float *m_data = nullptr;
};
