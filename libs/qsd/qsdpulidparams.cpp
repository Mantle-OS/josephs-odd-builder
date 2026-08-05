#include "qsdpulidparams.h"

QSdPulidParams::QSdPulidParams(QObject *parent):
    QSdBaseParam{parent}
{
    resetPulidParams();
}

QSdPulidParams::~QSdPulidParams()
{
}

sd_pulid_params_t QSdPulidParams::pulidParams()
{
    sd_pulid_params_t ret{};

    tmp_idEmbeddingPath = m_idEmbeddingPath.toLocal8Bit();
    ret.id_embedding_path = tmp_idEmbeddingPath.isEmpty() ? nullptr : tmp_idEmbeddingPath.constData();
    ret.id_weight = m_idWeight;
    m_pulid_params = ret;
    return m_pulid_params;
}

void QSdPulidParams::setPulidParams(const sd_pulid_params_t &other)
{
    set_idEmbeddingPath(other.id_embedding_path ? QString::fromUtf8(other.id_embedding_path) : QString{});
    set_idWeight(other.id_weight);
    m_pulid_params = other;
}

void QSdPulidParams::resetPulidParams()
{
    m_pulid_params = {nullptr, 1.0f};
    setPulidParams(m_pulid_params);
}


