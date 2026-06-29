#include "qllamaclientengine.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include <QFutureWatcher>

QLlamaClientEngine::QLlamaClientEngine(QObject *parent) :
    QLlamaEngine{parent},
    m_nam(new QNetworkAccessManager(this))
{
}

QLlamaClientEngine::~QLlamaClientEngine()
{
    cancel();
    delete m_nam;
}

void QLlamaClientEngine::generate()
{
    if (get_prompt().isEmpty())
        return;

    QPromise<QString> promise;
    auto future = promise.future();

    m_requestQueue.enqueue(new StreamReqItem{get_prompt(), std::move(promise)});

    connect(this, &QLlamaClientEngine::generationStarted, this, [this, future]() mutable {
        auto *watcher = new QFutureWatcher<QString>(this);

        connect(watcher, &QFutureWatcher<QString>::resultReadyAt, this, [this, watcher](int index) {
            QString tokenChunk = watcher->future().resultAt(index);
            set_streamingText(get_streamingText() + tokenChunk);
            Q_EMIT tokenStreamed(tokenChunk);
        });

        connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
            Q_EMIT generationFinished(get_streamingText());
            set_isProcessing(false);
            watcher->deleteLater();
            processNextNetworkRequest(); // Pull next item from the active queue pipeline
        });

        watcher->setFuture(future);
    });

    if (!get_isProcessing()) {
        set_isProcessing(true);
        set_streamingText("");
        Q_EMIT generationStarted();
        processNextNetworkRequest();
    }
}

void QLlamaClientEngine::processNextNetworkRequest()
{
    if (m_requestQueue.isEmpty()) {
        set_isProcessing(false);
        return;
    }

    m_currentReqItem = m_requestQueue.dequeue();
    m_networkBuffer.clear();

    // Build the standardized(finding more docs on this is tough) OpenAI-"compatible" streaming payload document
    QJsonObject rootObj;
    rootObj["model"] = get_modelName();
    rootObj["temperature"] = get_temperature();
    rootObj["top_p"] = get_topP();
    rootObj["max_tokens"] = get_maxTokens();
    rootObj["stream"] = true; // Crucial layout configuration multiplier

    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = m_currentReqItem->promptPayload;

    QJsonArray messagesArray;
    messagesArray.append(messageObj);
    rootObj["messages"] = messagesArray;

    QNetworkRequest request((QUrl(get_endpointUrl())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!get_apiKey().isEmpty())
        request.setRawHeader("Authorization", QString("Bearer %1").arg(get_apiKey()).toUtf8());

    m_currentReply = m_nam->post(request, QJsonDocument(rootObj).toJson());
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, &QLlamaClientEngine::handleReadyRead);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &QLlamaClientEngine::handleReplyFinished);
}

void QLlamaClientEngine::handleReadyRead()
{
    if (!m_currentReply || !m_currentReqItem)
        return;

    // Read the chunk bytes as they travel over the ?wire? not sure what it is really called(tech term)
    m_networkBuffer.append(m_currentReply->readAll());

    int newlineIndex = m_networkBuffer.indexOf('\n');
    while (newlineIndex != -1) {
        QByteArray line = m_networkBuffer.left(newlineIndex).trimmed();
        m_networkBuffer.remove(0, newlineIndex + 1);
        newlineIndex = m_networkBuffer.indexOf('\n');

        if (line.startsWith("data: ")) {
            QByteArray jsonPayload = line.mid(6).trimmed();

            if (jsonPayload == "[DONE]")
                break; // Stop pass encountered safely

            QJsonDocument doc = QJsonDocument::fromJson(jsonPayload);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject choicesObj = doc.object()["choices"].toArray().first().toObject();
                QString textToken = choicesObj["delta"].toObject()["content"].toString();

                // Push the token chunk down the promise future NOW
                if (!textToken.isEmpty())
                    m_currentReqItem->promise.addResult(textToken);
            }
        }
    }
}

void QLlamaClientEngine::handleReplyFinished()
{
    if (m_currentReqItem) {
        m_currentReqItem->promise.finish();
        delete m_currentReqItem;
        m_currentReqItem = nullptr;
    }

    if (m_currentReply) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void QLlamaClientEngine::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    if (m_currentReqItem) {
        m_currentReqItem->promise.finish();
        delete m_currentReqItem;
        m_currentReqItem = nullptr;
    }

    // Clean out the remainder of the pending request tree
    while (!m_requestQueue.isEmpty()) {
        auto *item = m_requestQueue.dequeue();
        item->promise.finish();
        delete item;
    }

    set_isProcessing(false);
}
