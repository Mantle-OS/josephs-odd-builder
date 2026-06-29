#ifndef QSDEMBEDDING_H
#define QSDEMBEDDING_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"

class QSdEmbedding : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QString,  embeddingName,  {}        )
    QP_RW(QString,  embeddingPath,  {}        )
    QP_RW(bool,     isEnabled,      false     )

    QML_ELEMENT
public:
    explicit QSdEmbedding(QObject *parent = nullptr) ;
    ~QSdEmbedding();

    sd_embedding_t embeddings();
    void setEmbeddings(sd_embedding_t other);
    void resetEmbeddings();

private:
    sd_embedding_t m_embeddings{nullptr, nullptr};
};

#endif // QSDEMBEDDING_H
