#include "qsdtilingparams.h"

QSdTilingParams::QSdTilingParams(QObject *parent):
    QSdBaseParam{parent}
{
    resetTilingParams();
}

QSdTilingParams::~QSdTilingParams()
{
    resetTilingParams();
}

sd_tiling_params_t QSdTilingParams::tilingParams()
{
    sd_tiling_params_t ret{};
    ret.enabled = m_isEnabled;
    ret.temporal_tiling = m_temporalTiling;
    ret.tile_size_x = m_tileSizeX;
    ret.tile_size_y = m_tileSizeY;
    ret.target_overlap = m_targetOverlap;
    ret.rel_size_x = m_relSizeX;
    ret.rel_size_y = m_relSizeY;

    if(!m_extraTilingArgs.isEmpty()){
        tmp_extraTilingArgs = m_extraTilingArgs.toLocal8Bit();
        ret.extra_tiling_args = tmp_extraTilingArgs.constData();
    }else{
        ret.extra_tiling_args = nullptr;
    }
    return ret;
}

void QSdTilingParams::setTilingParams(sd_tiling_params_t  other)
{
    set_isEnabled(other.enabled);
    set_temporalTiling(other.temporal_tiling);
    set_tileSizeX(other.tile_size_x);
    set_tileSizeY(other.tile_size_y);
    set_targetOverlap(other.target_overlap);
    set_relSizeX(other.rel_size_x);
    set_relSizeY(other.rel_size_y);
    set_extraTilingArgs(QString::fromLatin1(other.extra_tiling_args));
}

void QSdTilingParams::resetTilingParams()
{
    m_tilingParams = { false, false, 0, 0, 0.5f, 0.0f, 0.0f, nullptr };
    m_extraTilingArgs.clear();
}
