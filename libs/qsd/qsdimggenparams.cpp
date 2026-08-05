#include "qsdimggenparams.h"
QSdImgGenParams::QSdImgGenParams(QObject *parent) :
    QSdBaseParam{parent},
    m_loras{new ObjectListModel<QSdLora>{this, "display", "path"}},
    m_initImage{new QSdImage{}},
    m_refImages{new ObjectListModel<QSdImage>{this, "display", "sourcePath"}},
    m_maskImage{new QSdImage{}},
    m_sampleParams{new QSdSampleParams{this}},
    m_controlImage{new QSdImage{}},
    m_pmParams{new QSdPmParams{this}},
    m_pulidParams{new QSdPulidParams{this}},
    m_vaeTilingParams{new QSdTilingParams{this}},
    m_cache{new QSdCacheParams{this}},
    m_hires{new QSdHiResParams{this}}
{


    // m_sampleParams->setSampleParams(&m_imgGenParms->sample_params);
    // m_imgGenParms->sample_params = *m_sampleParams->sampleParams();
    // m_imgGenParms->pm_params = *m_pmParams->pmParams();
    // m_imgGenParms->cache = *m_cache->cacheParams();
    // m_imgGenParms->hires = *m_hires->hiresParams();
    // sd_img_gen_params_init(m_imgGenParms);
    resetImgGenParms();
}

QSdImgGenParams::~QSdImgGenParams()
{
    if(!m_loras->isEmpty())
        m_loras->clear();

    delete m_loras;
    m_loras = nullptr;


    // these are QQuickItems so they have no parent at all
    if(!m_refImages->isEmpty()){
        m_refImages->clear();
        m_proxyRefImages.clear();
    }
    delete m_refImages;
    m_refImages = nullptr;
    // multi image end

    if (m_initImage){
        delete m_initImage;
        m_initImage = nullptr;
    }

    if(m_maskImage){
        delete m_maskImage;
        m_maskImage = nullptr;
    }
    if(m_sampleParams){
        delete m_sampleParams;
        m_sampleParams = nullptr;
    }
    if(m_controlImage){
        delete m_controlImage;
        m_controlImage = nullptr;
    }

    if(m_pulidParams){
        delete m_pulidParams;
        m_pulidParams = nullptr;
    }

    if(m_pmParams){
        delete m_pmParams;
        m_pmParams = nullptr;
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
}

QString QSdImgGenParams::debugString()
{
    char *raw = sd_img_gen_params_to_str(&m_imgGenParms);
    QString ret = raw ? QString::fromUtf8(raw) : QStringLiteral("Unknown");
    free(raw);
    return ret;
}

sd_img_gen_params_t QSdImgGenParams::imgGenParms()
{
    sd_img_gen_params_t ret{};
    m_loraVec.clear();
    if (m_loras) {
        m_loraVec.reserve(m_loras->size());
        for (QSdLora *i : m_loras->toList())
            if (i)
                m_loraVec.push_back(i->lora());
    }

    ret.loras      = m_loraVec.empty() ? nullptr : m_loraVec.data();
    ret.lora_count = static_cast<uint32_t>(m_loraVec.size());


    if (!m_prompt.isEmpty()) {
        tmp_prompt = m_prompt.toUtf8();
        ret.prompt = tmp_prompt.constData();
    } else {
        ret.prompt = nullptr;
    }

    if (!m_negativePrompt.isEmpty()) {
        tmp_negativePrompt = m_negativePrompt.toUtf8();
        ret.negative_prompt = tmp_negativePrompt.constData();
    } else {
        ret.negative_prompt = nullptr;
    }

    ret.clip_skip             = m_clipSkip;
    ret.init_image            = m_initImage ? m_initImage->img() : sd_image_t{};

    if(m_refImages->isEmpty()){
        ret.ref_images            = nullptr;
        ret.ref_images_count      = 0;
    }else{
        m_proxyRefImages.clear();
        for (QSdImage *i : m_refImages->toList())
            if(i)
                m_proxyRefImages.push_back(i->img());

        ret.ref_images = m_proxyRefImages.empty() ? nullptr : m_proxyRefImages.data();
        ret.ref_images_count = static_cast<uint32_t>(m_proxyRefImages.size());
    }

    ret.auto_resize_ref_image = m_autoResizeRefImage;
    ret.increase_ref_index    = m_increaseRefIndex;
    ret.mask_image            = m_maskImage ? m_maskImage->img() : sd_image_t{};
    ret.width                 = m_imgWidth;
    ret.height                = m_imgHeight;
    ret.sample_params         = m_sampleParams ? m_sampleParams->sampleParams() : sd_sample_params_t{};
    ret.strength              = m_strength;
    ret.seed                  = m_seed;
    ret.batch_count           = m_batchCount;
    ret.control_image         = m_controlImage ? m_controlImage->img() : sd_image_t{};
    ret.control_strength      = m_controlStrength;
    ret.pulid_params          = m_pulidParams ? m_pulidParams->pulidParams() : sd_pulid_params_t{};
    ret.pm_params             = m_pmParams ? m_pmParams->pmParams() : sd_pm_params_t{};
    ret.vae_tiling_params     = m_vaeTilingParams ? m_vaeTilingParams->tilingParams() : sd_tiling_params_t{};
    ret.cache                 = m_cache ? m_cache->cacheParams() : sd_cache_params_t{};
    ret.hires                 = m_hires ? m_hires->hiresParams() : sd_hires_params_t{};

    m_imgGenParms = ret;

    return ret;
}

void QSdImgGenParams::setImgGenParms(sd_img_gen_params_t other)
{

    if (other.loras && other.lora_count > 0 && m_loras) {
        for (uint32_t i = 0; i < other.lora_count; i++) {

            if (!other.loras[i].path)
                continue;

            QString lora_fileName = QString::fromUtf8(other.loras[i].path, strnlen(other.loras[i].path, 2048));

            QSdLora *nLora = m_loras->getByUid(lora_fileName);
            if (nLora == Q_NULLPTR) {
                nLora = new QSdLora(m_loras);
                m_loras->append(nLora);
            }
            nLora->setLora(other.loras[i]);
        }
    }


    set_prompt(other.prompt ? QString::fromUtf8(other.prompt) : QString{});
    set_negativePrompt(other.negative_prompt ? QString::fromUtf8(other.negative_prompt): QString{});

    set_clipSkip(other.clip_skip);

    m_initImage->setImg(other.init_image);
    if(other.ref_images && other.ref_images_count > 0){
        set_refImagesCount(other.ref_images_count);
        m_proxyRefImages.clear();
        if(!m_refImages->isEmpty())
            m_refImages->clear();

        for (uint32_t i = 0; i < other.ref_images_count; i++) {
            QSdImage *rImg  = new QSdImage{}; // m_refImages->getByUid()
            rImg->setImg(other.ref_images[i]);
            m_refImages->append(rImg);
        }

    }else{
        if(!m_refImages->isEmpty())
            m_proxyRefImages.clear();
        set_refImagesCount(0);
    }

    set_autoResizeRefImage(other.auto_resize_ref_image);
    set_increaseRefIndex(other.increase_ref_index);

    m_maskImage->setImg(other.mask_image);

    set_imgHeight(other.height);
    set_imgWidth(other.width);

    m_sampleParams->setSampleParams(other.sample_params);
    set_strength(other.strength);
    set_seed(other.seed);
    set_batchCount(other.batch_count);

    m_controlImage->setImg(other.control_image);
    set_controlStrength(other.control_strength);

    m_pulidParams->setPulidParams(other.pulid_params);
    m_pmParams->setPmParams(other.pm_params);
    m_vaeTilingParams->setTilingParams(other.vae_tiling_params);
    m_cache->setCacheParams(other.cache);
    m_hires->setHiresParams(other.hires);
    m_imgGenParms = other;
}

void QSdImgGenParams::resetImgGenParms()
{
    if(!m_loras->isEmpty())
        m_loras->clear();
    m_imgGenParms.loras                     = nullptr;
    m_imgGenParms.lora_count                = 0;

    m_imgGenParms.prompt                    = nullptr;
    m_imgGenParms.negative_prompt           = nullptr;
    m_imgGenParms.clip_skip                 = -1;

    m_imgGenParms.init_image                = m_initImage->img();

    if(!m_refImages->isEmpty())
        m_refImages->clear();

    m_imgGenParms.ref_images                = nullptr;
    m_imgGenParms.ref_images_count          = 0;

    m_imgGenParms.auto_resize_ref_image     = true;
    m_imgGenParms.increase_ref_index        = false;

    m_imgGenParms.mask_image                = m_maskImage->img();
    m_imgGenParms.width                     = 512;
    m_imgGenParms.height                    = 512;

    m_sampleParams->resetSampleParams();
    m_imgGenParms.sample_params = m_sampleParams->sampleParams();

    m_imgGenParms.strength                  = 0.75f;
    m_imgGenParms.seed                      = -1;
    m_imgGenParms.batch_count               = 1;

    m_imgGenParms.control_image = m_controlImage->img();
    m_imgGenParms.control_strength          = 0.9f;

    m_pulidParams->resetPulidParams();
    m_imgGenParms.pulid_params              = m_pulidParams->pulidParams();

    m_pmParams->resetPmParams();
    m_imgGenParms.pm_params                 = m_pmParams->pmParams();

    m_vaeTilingParams->resetTilingParams();
    m_imgGenParms.vae_tiling_params         = m_vaeTilingParams->tilingParams();

    m_cache->resetCacheParams();
    m_imgGenParms.cache                     = m_cache->cacheParams();

    m_hires->resetHiresParams();
    m_imgGenParms.hires                     = m_hires->hiresParams();

    setImgGenParms(m_imgGenParms);

}
