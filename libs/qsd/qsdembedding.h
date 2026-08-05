#pragma once

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include "qsdbaseparam.h"

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdEmbedding : public QSdBaseParam
{
    Q_OBJECT
    QP_RW(QString,  embeddingName,  {}        ) // The trigger word used in the prompt to activate this textual inversion.
    QP_RW(QString,  embeddingPath,  {}        ) // The file path to the embedding model (.pt, .bin, or .safetensors).
    QP_RW(bool,     isEnabled,      false     ) // [Local wrapper only] Toggle to bypass this embedding without removing it from the UI list.

    QML_ELEMENT
public:
    explicit QSdEmbedding(QObject *parent = nullptr) ;
    ~QSdEmbedding();

    sd_embedding_t embeddings();
    void setEmbeddings(sd_embedding_t other);
    void resetEmbeddings();

private:
    sd_embedding_t m_embeddings{nullptr, nullptr};
    QByteArray  tmp_embeddingName;
    QByteArray  tmp_embeddingPath;
};

