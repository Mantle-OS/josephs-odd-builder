#include "qsdslgparams.h"

QSdSlgParams::QSdSlgParams(QObject *parent) :
    QSdBaseParam{parent}
{
    resetSlgParams();
}

QSdSlgParams::~QSdSlgParams()
{
    resetSlgParams();

    // I am not sure about ownershitp here some back later to this
    // FIXME
    if(m_layers)
        m_layers = nullptr;
}

sd_slg_params_t QSdSlgParams::slgParams() const
{
    sd_slg_params_t slgParams = {};
    slgParams.layers = m_layers;
    slgParams.layer_count = m_layerCount;
    slgParams.layer_start = m_layerStart;
    slgParams.layer_end = m_layerEnd;
    return slgParams;
}

void QSdSlgParams::setSlgParams(sd_slg_params_t other)
{
    m_layers      = other.layers;
    set_layerCount( other.layer_count);
    set_layerStart( other.layer_start);
    set_layerEnd(   other.layer_end);
    set_scale(      other.scale);

    m_slgParams = other;
}

void QSdSlgParams::resetSlgParams()
{
    m_layers = nullptr;
    m_slgParams = { nullptr, 0,  0.01f, 0.2f, 0.f };
    setSlgParams(m_slgParams);
}


