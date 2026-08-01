#include "qllamaserverengine.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

#include <llama.h>

QLlamaServerEngine::QLlamaServerEngine(QObject *parent) :
    QLlamaBase(parent),
    m_tcpServer(new QTcpServer(this))
{
}

QLlamaServerEngine::~QLlamaServerEngine()
{
    stopServer();
}

bool QLlamaServerEngine::startServer()
{
    if (m_tcpServer->isListening()) return true;

    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &QLlamaServerEngine::handleNewConnection);

    bool ok = m_tcpServer->listen(QHostAddress::Any, static_cast<quint16>(get_listenPort()));
    set_isListening(ok);

    if (ok)
        qDebug() << "[qllama-server] API running on port:" << get_listenPort();

    return ok;
}
void QLlamaServerEngine::stopServer()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
        set_isListening(false);
        qDebug() << "[qllama-server] API stopped";
    }

    // Drop any trailing client connection footprints cleanly
    for (auto *socket : m_clientBuffers.keys()) {
        socket->disconnect();
        socket->close();
        socket->deleteLater();
    }
    m_clientBuffers.clear();
}

void QLlamaServerEngine::handleNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
        m_clientBuffers.insert(clientSocket, QByteArray());

        connect(clientSocket, &QTcpSocket::readyRead,
                this, &QLlamaServerEngine::handleClientReadyRead);

        connect(clientSocket, &QTcpSocket::disconnected,
                this, &QLlamaServerEngine::handleClientDisconnected);

        qDebug() << "[qllama-server] Incoming client connection accepted from descriptor handle:" << clientSocket->socketDescriptor();
    }
}

void QLlamaServerEngine::handleClientReadyRead()
{
    auto *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    m_clientBuffers[clientSocket].append(clientSocket->readAll());
    QByteArray &buffer = m_clientBuffers[clientSocket];

    qDebug() << "[qllama-server] readyRead. buffer size:" << buffer.size();

    if (buffer.contains("\r\n\r\n")) {
        int headerEnd = buffer.indexOf("\r\n\r\n");

        int contentLength = 0;
        int clIndex = buffer.indexOf("Content-Length:");
        if (clIndex != -1 && clIndex < headerEnd) {
            int lineEnd = buffer.indexOf("\r\n", clIndex);
            QByteArray clLine = buffer.mid(clIndex + 15, lineEnd - (clIndex + 15)).trimmed();
            contentLength = clLine.toInt();
        }

        qDebug() << "[qllama-server] Headers detected. Content-Length parsed:" << contentLength
                 << "Expected total frame size:" << (headerEnd + 4 + contentLength);

        if (buffer.size() >= headerEnd + 4 + contentLength) {
            qDebug() << "[qllama-server] Full payload frame matched boundary. Proceeding to parse.";
            parseIncomingHttpRequest(clientSocket, buffer);
        } else {
            qDebug() << "[qllama-server] Frame incomplete. Waiting for more chunks over socket ring...";
        }
    } else {
        qDebug() << "[qllama-server] Incomplete HTTP headers. Waiting for end-of-header delimiter...";
    }
}

void QLlamaServerEngine::parseIncomingHttpRequest(QTcpSocket *clientSocket, const QByteArray &rawBuffer)
{
    int headerEnd = rawBuffer.indexOf("\r\n\r\n");
    QByteArray headers = rawBuffer.left(headerEnd);
    QByteArray body = rawBuffer.mid(headerEnd + 4);

    if (!headers.startsWith("POST")) {
        qWarning() << "[qllama-server] Rejecting Request: Non-POST method received.";
        clientSocket->write("HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n");
        clientSocket->disconnectFromHost();
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[qllama-server] Rejecting Request: Body JSON payload failed to parse. Error:" << err.errorString();
        qDebug() << "[qllama-server] Raw rejected body text was:" << body;
        clientSocket->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        clientSocket->disconnectFromHost();
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray messages = rootObj["messages"].toArray();
    if (messages.isEmpty()) {
        qWarning() << "[qllama-server] Rejecting Request: 'messages' array is missing or empty.";
        clientSocket->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        clientSocket->disconnectFromHost();
        return;
    }

    QString extractedPrompt = messages.last().toObject()["content"].toString();
    qDebug() << "[qllama-server] Verification success. Prompt extracted length:" << extractedPrompt.length();

    m_clientBuffers[clientSocket].clear();
    executeStreamingInference(clientSocket, extractedPrompt);
}

void QLlamaServerEngine::executeStreamingInference(QTcpSocket *clientSocket, const QString &promptPayload)
{
    qDebug() << "[qllama-server] Entering executeStreamingInference. Payload:" << promptPayload;

    auto *model = get_model();
    auto *ctx = get_context();
    auto *smpl = get_sampler();

    qDebug() << "[qllama-server] Engine Component Check -> Model:" << model
             << " | Context:" << ctx
             << " | Sampler:" << smpl
             << " | m_isGenerating:" << m_isGenerating;

    if (!model || !ctx || !smpl || m_isGenerating) {
        qWarning() << "[qllama-server] Aborting inference: Core engine components missing or busy.";
        clientSocket->write("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n");
        clientSocket->disconnectFromHost();
        return;
    }

    const struct llama_vocab* nativeVocab = model->nativeVocab();
    struct llama_context* nativeCtx = ctx->nativeContext();

    qDebug() << "[qllama-server] Native Handle Check -> nativeVocab:" << nativeVocab
             << " | nativeCtx:" << nativeCtx;

    if (!nativeVocab || !nativeCtx) {
        qWarning() << "[qllama-server] Aborting inference: Underlying llama.cpp native context handles are NULL.";
        clientSocket->disconnectFromHost();
        return;
    }

    m_isGenerating = true;

    // Send HTTP SSE streaming headers down the pipe
    clientSocket->write("HTTP/1.1 200 OK\r\n");
    clientSocket->write("Content-Type: text/event-stream\r\n");
    clientSocket->write("Cache-Control: no-cache\r\n");
    clientSocket->write("Connection: keep-alive\r\n");
    clientSocket->write("Transfer-Encoding: chunked\r\n\r\n");
    clientSocket->flush();

    // Tokenize prompt payload string via global wrapper
    QByteArray promptBytes = promptPayload.toUtf8();
    std::vector<llama_token> tokens(promptBytes.size() + 4);
    int32_t n_tokens = ::llama_tokenize(nativeVocab, promptBytes.constData(), promptBytes.size(),
                                        tokens.data(), tokens.size(), true, true);

    qDebug() << "[qllama-server] Prompt tokenization complete. Generated tokens count:" << n_tokens;

    if (n_tokens >= 0) {
        tokens.resize(n_tokens);
    } else {
        qWarning() << "[qllama-server] Tokenization failed with return code:" << n_tokens;
        clientSocket->disconnectFromHost();
        m_isGenerating = false;
        return;
    }

    // Flush the abstract cell tracks without wiping structural graph setups
    llama_memory_t mem = ::llama_get_memory(nativeCtx);
    if (mem)
        ::llama_memory_clear(mem, false);

    // Initialize a local, clean batch container dedicated to this network call pass
    struct llama_batch localBatch = ::llama_batch_init(static_cast<int32_t>(tokens.size() + 4), 0, 1);

    int32_t kvTokensUsed = 0;
    int32_t tokensGeneratedCount = 0;
    int32_t maxCtx = 2048;

    // Load initial prompt sequence array blocks directly into the local batch
    localBatch.n_tokens = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        localBatch.token[localBatch.n_tokens]     = tokens[i];
        localBatch.pos[localBatch.n_tokens]       = kvTokensUsed + localBatch.n_tokens;
        localBatch.n_seq_id[localBatch.n_tokens]  = 1;
        localBatch.seq_id[localBatch.n_tokens][0] = 0;
        localBatch.logits[localBatch.n_tokens]    = (i == tokens.size() - 1);
        localBatch.n_tokens++;
    }

    // FIXME: Using local variable pointer 'smpl' instead of 'sampler'
    smpl->setVocabContextSize(::llama_vocab_n_tokens(nativeVocab));
    struct llama_sampler* nativeChain = smpl->prepareSamplerChain();

    qDebug() << "[qllama-server] Native Sampler Chain initialized:" << nativeChain;
    if (!nativeChain) {
        qWarning() << "[qllama-server] Aborting inference: Native Sampler Chain is NULL.";
        ::llama_batch_free(localBatch);
        clientSocket->disconnectFromHost();
        m_isGenerating = false;
        return;
    }

    qDebug() << "[qllama-server] Entering generation loop engine execution sweep...";

    while (kvTokensUsed < maxCtx && tokensGeneratedCount < 512) {
        int decodeStatus = ::llama_decode(nativeCtx, localBatch);
        if (decodeStatus != 0) {
            qWarning() << "[qllama-server] Internal loop decode failure code:" << decodeStatus;
            break;
        }

        kvTokensUsed += localBatch.n_tokens;
        ctx->setProperty("kvTokensUsed", kvTokensUsed);

        // Sample token candidate out OF active logits plane array
        llama_token predictedToken = ::llama_sampler_sample(nativeChain, nativeCtx, -1);
        if (predictedToken < 0 || ::llama_vocab_is_eog(nativeVocab, predictedToken)) {
            qDebug() << "[qllama-server] Stream reached completion token ID boundary:" << predictedToken;
            break;
        }

        char pieceBuf[256];
        int32_t pieceLen = ::llama_token_to_piece(nativeVocab, predictedToken, pieceBuf, sizeof(pieceBuf), 0, true);
        if (pieceLen > 0) {
            QString chunk = QString::fromUtf8(pieceBuf, pieceLen);
            tokensGeneratedCount++;

            QJsonObject respDoc;
            QJsonObject choicesObj;
            QJsonObject deltaObj;

            deltaObj["content"] = chunk;
            choicesObj["delta"] = deltaObj;

            QJsonArray choicesArr;
            choicesArr.append(choicesObj);
            respDoc["choices"] = choicesArr;

            QByteArray ssePayload = "data: " + QJsonDocument(respDoc).toJson(QJsonDocument::Compact) + "\n\n";
            QByteArray chunkHeader = QByteArray::number(ssePayload.size(), 16) + "\r\n";

            if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState) {
                clientSocket->write(chunkHeader + ssePayload + "\r\n");
                clientSocket->flush();
            } else {
                qWarning() << "[qllama-server] Client disconnected mid-stream.";
                break;
            }
        }

        localBatch.n_tokens = 0;
        localBatch.token[0]     = predictedToken;
        localBatch.pos[0]       = kvTokensUsed;
        localBatch.n_seq_id[0]  = 1;
        localBatch.seq_id[0][0] = 0;
        localBatch.logits[0]    = 1;
        localBatch.n_tokens     = 1;
    }

    qDebug() << "[qllama-server] Generation loop exited. Total tokens streamed:" << tokensGeneratedCount;

    ::llama_batch_free(localBatch);

    if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write("0\r\n\r\n");
        clientSocket->flush();
        clientSocket->disconnectFromHost();
    }

    m_isGenerating = false;
}


void QLlamaServerEngine::handleClientDisconnected()
{
    auto *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        m_clientBuffers.remove(clientSocket);
        clientSocket->deleteLater();
        qDebug() << "[qllama-server] Client disconnected. Cleaning footprint structures.";
    }
}
