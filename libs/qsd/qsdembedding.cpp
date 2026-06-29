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
        const std::string c_name = m_embeddingName.toStdString();
        ret.name = c_name.c_str();
    }

    if(!m_embeddingPath.isEmpty()){
        const std::string cpath = m_embeddingPath.toStdString();
        ret.path = cpath.c_str();
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
