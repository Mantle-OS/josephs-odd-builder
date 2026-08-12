#include "qsdguidanceparams.h"


QSdGuidanceParams::QSdGuidanceParams(QObject *parent) :
    QSdBaseParam{parent},
    m_slg{new QSdSlgParams{this}}
{
    resetGuidanceParams();
}

QSdGuidanceParams::~QSdGuidanceParams()
{

    m_guidanceParams.txt_cfg = 7.0f;
    m_guidanceParams.img_cfg = job::core::safeInfinity(); /// qInf();
    m_guidanceParams.distilled_guidance = 3.5f;

    if(m_slg)
        delete m_slg;
    m_slg = nullptr;
}

sd_guidance_params_t QSdGuidanceParams::guidanceParams()
{
    sd_guidance_params_t guidance = {};
    guidance.txt_cfg            = m_txtCfg;
    guidance.img_cfg            = m_imgCfg;
    guidance.distilled_guidance = m_distilledGuidance;
    guidance.slg = m_slg->slgParams();
    return guidance;
}

void QSdGuidanceParams::setGuidanceParams(sd_guidance_params_t other)
{
    set_txtCfg(other.txt_cfg);
    set_imgCfg(other.img_cfg);
    set_distilledGuidance(other.distilled_guidance);
    m_slg->setSlgParams(other.slg);

    m_guidanceParams = other;
}

void QSdGuidanceParams::resetGuidanceParams()
{
    set_txtCfg(7.0f);
    // set_imgCfg(qInf());
    set_imgCfg(job::core::safeInfinity());
    set_distilledGuidance(3.5f);
    m_slg->resetSlgParams();
}
