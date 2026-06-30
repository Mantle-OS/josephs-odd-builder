#ifndef QLLAMACLIENTENGINE_H
#define QLLAMACLIENTENGINE_H

#include "qllamaengine.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QFuture>
#include <QPromise>
#include <QQueue>

#include <property-macros.h>

class QLlamaClientEngine : public QLlamaEngine
{
    Q_OBJECT

    QP_RW(QString, endpointUrl,  "http://127.0.0.1:8080/v1/chat/completions")
    QP_RW(QString, apiKey,       "")
    QP_RW(QString, modelName,    "qwen3-4b")

    QP_RW(double,  temperature,   0.75)
    QP_RW(double,  topP,          0.90)
    QP_RW(qint32,  maxTokens,     2048)

    QML_ELEMENT
public:
    explicit QLlamaClientEngine(QObject *parent = nullptr);
    ~QLlamaClientEngine() override;

    void generate() override;
    void cancel() override;

protected Q_SLOTS:
    void handleReadyRead();
    void handleReplyFinished();

private:
    struct StreamReqItem {
        QString           promptPayload;
        QPromise<QString> promise;
    };

    void processNextNetworkRequest();
    // FIXME there is cross thread waky things goiing on I must rebuild replay but this works for now as it is one then another
    QNetworkAccessManager *m_nam              = nullptr;
    QNetworkReply         *m_currentReply     = nullptr;
    StreamReqItem         *m_currentReqItem   = nullptr;
    QQueue<StreamReqItem*> m_requestQueue;

    QByteArray m_networkBuffer;
};


#endif // QLLAMACLIENTENGINE_H