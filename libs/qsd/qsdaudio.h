#ifndef QSDAUDIO_H
#define QSDAUDIO_H

#include <QObject>
#include <QQmlEngine>
#include <stable-diffusion.h>
#include "qsdbaseparam.h"
class QSdAudio : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(quint32,  sampleRate,      0)
    QP_RW(quint32,  channels,        0)
    QP_RW(quint64,  sampleCount,     0)

    Q_PROPERTY(float *data READ data NOTIFY dataChanged FINAL)

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

#endif // QSDAUDIO_H
