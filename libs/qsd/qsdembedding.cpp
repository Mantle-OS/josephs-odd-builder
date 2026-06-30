#include "qsdembedding.h"

QSdEmbedding::QSdEmbedding(QObject *parent) :
    QSdBaseParam{parent}
{   
}

QSdEmbedding::~QSdEmbedding()
{
    m_embeddings = {nullptr, nullptr};
}

sd_embedding_t QSdEmbedding::embeddings()
{
    sd_embedding_t ret = {nullptr, nullptr};
    if(!m_embeddingName.isEmpty()){
        tmp_embeddingName = m_embeddingName.toLocal8Bit();
        ret.name = tmp_embeddingName.constData();
    }

    if(!m_embeddingPath.isEmpty()){
        tmp_embeddingPath = m_embeddingPath.toLocal8Bit();
        ret.path = tmp_embeddingPath.constData();
    }

    return ret;
}

void QSdEmbedding::setEmbeddings(sd_embedding_t other)
{
    if(other.name){
        set_embeddingName(QString::fromLatin1(other.name));
    } else {
        set_embeddingName("");
    }

    if(other.path){
        set_embeddingPath(QString::fromLatin1(other.path));
    } else {
        set_embeddingPath("");
    }
}

void QSdEmbedding::resetEmbeddings()
{
    m_embeddings = {nullptr, nullptr};
    set_isEnabled(false);
}
