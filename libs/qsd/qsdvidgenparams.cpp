// qsdvidgenparams.cpp
#include "qsdvidgenparams.h"

QSdVidGenParams::QSdVidGenParams(QObject *parent) :
    QSdBaseParam{parent},
    m_loras{new ObjectListModel<QSdLora>{this, "display", "path"}},
    m_initImage{new QSdImage{}},
    m_endImage{new QSdImage{}},
    m_controlFrames{new ObjectListModel<QSdImage>{this, "display", "sourcePath"}},
    m_sampleParams{new QSdSampleParams{this}},
    m_highNoiseSampleParams{new QSdSampleParams{this}},
    m_vaeTilingParams{new QSdTilingParams{this}},
    m_cache{new QSdCacheParams{this}},
    m_hires{new QSdHiResParams{this}}
{
    resetVidGenParams();
}

QSdVidGenParams::~QSdVidGenParams()
{
    if(m_loras)         m_loras->clear();
    if(m_controlFrames) m_controlFrames->clear();

    delete m_loras;          m_loras = nullptr;
    delete m_controlFrames;  m_controlFrames = nullptr;

    if (m_initImage) {
        delete m_initImage;
        m_initImage = nullptr;
    }
    if (m_endImage) {
        delete m_endImage;
        m_endImage = nullptr;
    }
    if (m_sampleParams) {
        delete m_sampleParams;
        m_sampleParams = nullptr;
    }
    if (m_highNoiseSampleParams) {
        delete m_highNoiseSampleParams;
        m_highNoiseSampleParams = nullptr;
    }
    if (m_vaeTilingParams) {
        delete m_vaeTilingParams;
        m_vaeTilingParams = nullptr;
    }
    if (m_cache)                 {
        delete m_cache;
        m_cache = nullptr;
    }
    if (m_hires) {
        delete m_hires;
        m_hires = nullptr;
    }
    if (m_vidGenParams) {
        delete m_vidGenParams;
        m_vidGenParams = nullptr;
    }

}

sd_vid_gen_params_t *QSdVidGenParams::vidGenParams()
{
    sd_vid_gen_params_t ret{};

    m_loraVec.clear();
    if (m_loras) {
        m_loraVec.reserve(m_loras->size());
        for (QSdLora *i : m_loras->toList())
            if (i) m_loraVec.push_back(i->lora());
    }
    ret.loras      = m_loraVec.empty() ? nullptr : m_loraVec.data();
    ret.lora_count = static_cast<uint32_t>(m_loraVec.size());

    tmp_prompt = m_prompt.isEmpty() ? QByteArray{} : m_prompt.toUtf8();
    ret.prompt = tmp_prompt.isEmpty() ? nullptr : tmp_prompt.constData();

    tmp_negativePrompt = m_negativePrompt.isEmpty() ? QByteArray{} : m_negativePrompt.toUtf8();
    ret.negative_prompt = tmp_negativePrompt.isEmpty() ? nullptr : tmp_negativePrompt.constData();

    ret.clip_skip = m_clipSkip;
    ret.init_image = m_initImage ? m_initImage->img() : sd_image_t{};
    ret.end_image  = m_endImage  ? m_endImage->img()  : sd_image_t{};

    m_proxyControlFrames.clear();
    if (m_controlFrames && !m_controlFrames->isEmpty()) {
        for (QSdImage *i : m_controlFrames->toList())
            if (i) m_proxyControlFrames.push_back(i->img());
        ret.control_frames = m_proxyControlFrames.empty() ? nullptr : m_proxyControlFrames.data();
        ret.control_frames_size = static_cast<int>(m_proxyControlFrames.size());
    } else {
        ret.control_frames = nullptr;
        ret.control_frames_size = 0;
    }

    ret.width  = m_width;
    ret.height = m_height;
    ret.sample_params            = m_sampleParams          ? m_sampleParams->sampleParams()          : sd_sample_params_t{};
    ret.high_noise_sample_params = m_highNoiseSampleParams ? m_highNoiseSampleParams->sampleParams() : sd_sample_params_t{};
    ret.moe_boundary = m_moeBoundary;
    ret.strength     = m_strength;
    ret.seed         = m_seed;
    ret.video_frames = m_videoFrames;
    ret.fps          = m_fps;
    ret.vace_strength = m_vaceStrength;
    ret.vae_tiling_params = m_vaeTilingParams ? m_vaeTilingParams->tilingParams() : sd_tiling_params_t{};
    ret.cache             = m_cache          ? m_cache->cacheParams()             : sd_cache_params_t{};
    ret.hires             = m_hires          ? m_hires->hiresParams()             : sd_hires_params_t{};

    if (m_vidGenParams)
        *m_vidGenParams = ret;

    return m_vidGenParams;
}

void QSdVidGenParams::setVidGenParams(sd_vid_gen_params_t *other)
{
    if (!other)
        return;

    if (other->loras && other->lora_count > 0 && m_loras) {
        for (uint32_t i = 0; i < other->lora_count; i++) {
            if (!other->loras[i].path) continue;
            QString loraFileName = QString::fromUtf8(other->loras[i].path, strnlen(other->loras[i].path, 2048));
            QSdLora *nLora = m_loras->getByUid(loraFileName);
            if (nLora == Q_NULLPTR) {
                nLora = new QSdLora(m_loras);
                m_loras->append(nLora);
            }
            nLora->setLora(other->loras[i]);
        }
    }

    set_prompt(other->prompt ? QString::fromUtf8(other->prompt) : QString{});
    set_negativePrompt(other->negative_prompt ? QString::fromUtf8(other->negative_prompt) : QString{});
    set_clipSkip(other->clip_skip);

    m_initImage->setImg(other->init_image);
    m_endImage->setImg(other->end_image);

    m_controlFrames->clear();
    if (other->control_frames && other->control_frames_size > 0) {
        for (int i = 0; i < other->control_frames_size; i++) {
            QSdImage *frame = new QSdImage{};
            frame->setImg(other->control_frames[i]);
            m_controlFrames->append(frame);
        }
    }

    set_width(other->width);
    set_height(other->height);

    m_sampleParams->setSampleParams(other->sample_params);
    m_highNoiseSampleParams->setSampleParams(other->high_noise_sample_params);
    set_moeBoundary(other->moe_boundary);
    set_strength(other->strength);
    set_seed(other->seed);
    set_videoFrames(other->video_frames);
    set_fps(other->fps);
    set_vaceStrength(other->vace_strength);

    m_vaeTilingParams->setTilingParams(other->vae_tiling_params);
    m_cache->setCacheParams(other->cache);
    m_hires->setHiresParams(other->hires);

    if (m_vidGenParams && m_vidGenParams != other)
        *m_vidGenParams = *other;
}

void QSdVidGenParams::resetVidGenParams()
{
    if (!m_vidGenParams)
        return;
    sd_vid_gen_params_init(m_vidGenParams);
    setVidGenParams(m_vidGenParams);
}