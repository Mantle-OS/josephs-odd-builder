#ifndef QSDVIDGENPARAMS_H
#define QSDVIDGENPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>

#include "qsdcacheparams.h"
#include "qsdhiresparams.h"
#include "qsdimage.h"
#include "qsdlora.h"
#include "qsdsampleparams.h"
#include "qsdtilingparams.h"

class QSdVidGenParams : public QObject
{
    Q_OBJECT
    QP_PTR_RO(QSdLora, loras) // LOOK INTO MODEL ?
    Q_PROPERTY(quint32 loraCount READ loraCount WRITE setLoraCount NOTIFY loraCountChanged FINAL)
    Q_PROPERTY(QString prompt READ prompt WRITE setPrompt NOTIFY promptChanged FINAL)
    Q_PROPERTY(QString negativePrompt READ negativePrompt WRITE setNegativePrompt NOTIFY negativePromptChanged FINAL)
    Q_PROPERTY(int clipSkip READ clipSkip WRITE setClipSkip NOTIFY clipSkipChanged FINAL)

    QP_PTR_RW(QSdImage, initImage)
    QP_PTR_RO(QSdImage, endImage)
    QP_PTR_RO(QSdImage, controlFrames)

    Q_PROPERTY(int controlFramesSize READ controlFramesSize WRITE setControlFramesSize NOTIFY controlFramesSizeChanged FINAL)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged FINAL)

    QP_PTR_RW(QSdSampleParams, sampleParams)
    QP_PTR_RW(QSdSampleParams, highNoiseSampleParams)

    Q_PROPERTY(float moeBoundary READ moeBoundary WRITE setMoeBoundary NOTIFY moeBoundaryChanged FINAL)
    Q_PROPERTY(float strength READ strength WRITE setStrength NOTIFY strengthChanged FINAL)
    Q_PROPERTY(qint64 seed READ seed WRITE setSeed NOTIFY seedChanged FINAL)

    Q_PROPERTY(int videoFrames READ videoFrames WRITE setVideoFrames NOTIFY videoFramesChanged FINAL)
    Q_PROPERTY(int fps READ fps  WRITE setFps  NOTIFY fpsChanged FINAL)
    Q_PROPERTY(float vaceStrength READ vaceStrength WRITE setVaceStrength NOTIFY vaceStrengthChanged FINAL)

    QP_PTR_RO(QSdTilingParams, vaeTilingParams)
    QP_PTR_RO(QSdCacheParams, cache)
    QP_PTR_RO(QSdHiResParams, hires)

    QML_ELEMENT
public:
    explicit QSdVidGenParams(QObject *parent = nullptr);
    ~QSdVidGenParams();

    quint32 loraCount() const;
    void setLoraCount(quint32 newLoraCount);

    QString prompt() const;
    void setPrompt(const QString &newPrompt);

    QString negativePrompt() const;
    void setNegativePrompt(const QString &newNegativePrompt);

    int clipSkip() const;
    void setClipSkip(int newClipSkip);

    int controlFramesSize() const;
    void setControlFramesSize(int newControlFramesSize);

    int width() const;
    void setWidth(int newWidth);

    int height() const;
    void setHeight(int newHeight);

    float moeBoundary() const;
    void setMoeBoundary(float newMoeBoundary);

    float strength() const;
    void setStrength(float newStrength);

    qint64 seed() const;
    void setSeed(qint64 newSeed);

    int videoFrames() const;
    void setVideoFrames(int newVideoFrames);

    int fps() const;
    void setFps(int newFps);

    float vaceStrength() const;
    void setVaceStrength(float newVaceStrength);

Q_SIGNALS:
    void loraCountChanged();
    void promptChanged();
    void negativePromptChanged();
    void clipSkipChanged();
    void controlFramesSizeChanged();
    void widthChanged();
    void heightChanged();
    void moeBoundaryChanged();
    void strengthChanged();
    void seedChanged();
    void videoFramesChanged();
    void fpsChanged();
    void vaceStrengthChanged();

private:
    sd_vid_gen_params_t *m_vidGenParams = nullptr;
    QString m_prompt;
    QString m_negativePrompt;
};

#endif // QSDVIDGENPARAMS_H
