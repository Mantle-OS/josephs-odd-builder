#include "qsdvidgenparams.h"

QSdVidGenParams::QSdVidGenParams(QObject *parent) :
    m_loras{new QSdLora{this}},
    m_initImage{new QSdImage{}},
    m_endImage{new QSdImage{}},
    m_controlFrames{new QSdImage{}},
    m_sampleParams{new QSdSampleParams{this}},
    m_highNoiseSampleParams{new QSdSampleParams{this}},
    m_vaeTilingParams{new QSdTilingParams{this}},
    m_cache{new QSdCacheParams{this}},
    m_hires{new QSdHiResParams{this}},
    m_vidGenParams{new sd_vid_gen_params_t},
    QObject{parent}
{
    if(m_vidGenParams)
        sd_vid_gen_params_init(m_vidGenParams);
}

QSdVidGenParams::~QSdVidGenParams()
{
    if(m_loras){
        delete m_loras;
        m_loras = nullptr;
    }
    if (m_initImage){
        delete m_initImage;
        m_initImage = nullptr;
    }
    if(m_endImage){
        delete m_endImage;
        m_endImage = nullptr;
    }
    if(m_controlFrames){
        delete m_controlFrames;
        m_controlFrames = nullptr;
    }
    if(m_sampleParams){
        delete m_sampleParams;
        m_sampleParams = nullptr;
    }
    if(m_highNoiseSampleParams){
        delete m_highNoiseSampleParams;
        m_highNoiseSampleParams = nullptr;
    }

    if(m_vaeTilingParams){
        delete m_vaeTilingParams;
        m_vaeTilingParams = nullptr;
    }

    if(m_cache){
        delete m_cache;
        m_cache = nullptr;
    }

    if(m_hires){
        delete m_hires;
        m_hires = nullptr;
    }

    if(m_vidGenParams){
        delete m_vidGenParams;
        m_vidGenParams = nullptr;
    }

}

quint32 QSdVidGenParams::loraCount() const
{
    return m_vidGenParams->lora_count;
}

void QSdVidGenParams::setLoraCount(quint32 newLoraCount)
{
    if (m_vidGenParams->lora_count == newLoraCount) return;
    m_vidGenParams->lora_count = newLoraCount;
    Q_EMIT loraCountChanged();
}

QString QSdVidGenParams::prompt() const
{
    return m_prompt;
}

void QSdVidGenParams::setPrompt(const QString &newPrompt)
{
    if (m_prompt == newPrompt) return;
    m_prompt = newPrompt;
    m_vidGenParams->prompt = m_prompt.toLatin1().data();
    Q_EMIT promptChanged();
}

QString QSdVidGenParams::negativePrompt() const
{
    return m_negativePrompt;
}

void QSdVidGenParams::setNegativePrompt(const QString &newNegativePrompt)
{
    if (m_negativePrompt == newNegativePrompt) return;
    m_negativePrompt = newNegativePrompt;
    m_vidGenParams->negative_prompt = m_negativePrompt.toLatin1().data();
    Q_EMIT negativePromptChanged();
}

int QSdVidGenParams::clipSkip() const
{
    return m_vidGenParams->clip_skip;
}

void QSdVidGenParams::setClipSkip(int newClipSkip)
{
    if (m_vidGenParams->clip_skip == newClipSkip) return;
    m_vidGenParams->clip_skip = newClipSkip;
    Q_EMIT clipSkipChanged();
}

int QSdVidGenParams::controlFramesSize() const
{
    return m_vidGenParams->control_frames_size;
}

void QSdVidGenParams::setControlFramesSize(int newControlFramesSize)
{
    if (m_vidGenParams->control_frames_size == newControlFramesSize) return;
    m_vidGenParams->control_frames_size = newControlFramesSize;
    Q_EMIT controlFramesSizeChanged();
}

int QSdVidGenParams::width() const
{
    return m_vidGenParams->width;
}

void QSdVidGenParams::setWidth(int newWidth)
{
    if (m_vidGenParams->width == newWidth) return;
    m_vidGenParams->width = newWidth;
    Q_EMIT widthChanged();
}

int QSdVidGenParams::height() const
{
    return m_vidGenParams->height;
}

void QSdVidGenParams::setHeight(int newHeight)
{
    if (m_vidGenParams->height == newHeight) return;
    m_vidGenParams->height = newHeight;
    Q_EMIT heightChanged();
}

float QSdVidGenParams::moeBoundary() const
{
    return m_vidGenParams->moe_boundary;
}

void QSdVidGenParams::setMoeBoundary(float newMoeBoundary)
{
    if (qFuzzyCompare(m_vidGenParams->moe_boundary, newMoeBoundary)) return;
    m_vidGenParams->moe_boundary = newMoeBoundary;
    Q_EMIT moeBoundaryChanged();
}

float QSdVidGenParams::strength() const
{
    return m_vidGenParams->strength;
}

void QSdVidGenParams::setStrength(float newStrength)
{
    if (qFuzzyCompare(m_vidGenParams->strength, newStrength)) return;
    m_vidGenParams->strength = newStrength;
    Q_EMIT strengthChanged();
}

qint64 QSdVidGenParams::seed() const
{
    return m_vidGenParams->seed;
}

void QSdVidGenParams::setSeed(qint64 newSeed)
{
    if (m_vidGenParams->seed == newSeed) return;
    m_vidGenParams->seed = newSeed;
    Q_EMIT seedChanged();
}

int QSdVidGenParams::videoFrames() const
{
    return m_vidGenParams->video_frames;
}

void QSdVidGenParams::setVideoFrames(int newVideoFrames)
{
    if (m_vidGenParams->video_frames == newVideoFrames) return;
    m_vidGenParams->video_frames = newVideoFrames;
    Q_EMIT videoFramesChanged();
}

int QSdVidGenParams::fps() const
{
    return m_vidGenParams->fps;
}

void QSdVidGenParams::setFps(int newFps)
{
    if (m_vidGenParams->fps == newFps) return;
    m_vidGenParams->fps = newFps;
    Q_EMIT fpsChanged();
}

float QSdVidGenParams::vaceStrength() const
{
    return m_vidGenParams->vace_strength;
}

void QSdVidGenParams::setVaceStrength(float newVaceStrength)
{
    if (qFuzzyCompare(m_vidGenParams->vace_strength, newVaceStrength)) return;
    m_vidGenParams->vace_strength = newVaceStrength;
    Q_EMIT vaceStrengthChanged();
}
