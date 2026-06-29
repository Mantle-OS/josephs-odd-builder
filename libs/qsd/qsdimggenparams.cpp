#include "qsdimggenparams.h"
QSdImgGenParams::QSdImgGenParams(QObject *parent) :
    m_loras{new ObjectListModel<QSdLora>{this, "display", "path"}},
    m_initImage{new QSdImage{}},
    m_refImages{new QSdImage{}},
    m_maskImage{new QSdImage{}},
    m_sampleParams{new QSdSampleParams{this}},
    m_controlImage{new QSdImage{}},
    m_pmParams{new QSdPmParams{this}},
    m_vaeTilingParams{new QSdTilingParams{this}},
    m_cache{new QSdCacheParams{this}},
    m_hires{new QSdHiResParams{this}},
    QSdBaseParam{parent}
{


    // m_sampleParams->setSampleParams(&m_imgGenParms->sample_params);
    // m_imgGenParms->sample_params = *m_sampleParams->sampleParams();
    // m_imgGenParms->pm_params = *m_pmParams->pmParams();
    // m_imgGenParms->cache = *m_cache->cacheParams();
    // m_imgGenParms->hires = *m_hires->hiresParams();
    // sd_img_gen_params_init(m_imgGenParms);
}

QSdImgGenParams::~QSdImgGenParams()
{
    if(!m_loras->isEmpty()){
        m_loras->clear();
    }
    if(m_loras){
        delete m_loras;
        m_loras = nullptr;
    }

    if (m_initImage){
        delete m_initImage;
        m_initImage = nullptr;
    }
    if(m_refImages){
        delete m_refImages;
        m_refImages = nullptr;
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
    QString ret = "Unknown";
    ret = sd_img_gen_params_to_str(&m_imgGenParms);
    return ret;
}
