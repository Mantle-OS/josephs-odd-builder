#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QQueue>

#include <property-macros.h>
#include <pointer-macros.h>

#include "qllamabase.h"
#include "qllamamodel.h"
#include "qllmacontext.h"
#include "qllamasampler.h"

#include "qllama_export.h"
class QLLAMA_EXPORT QLlamaServerEngine : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT
    QP_RW(qint32,               listenPort,  8080   )
    QP_RO(bool,                 isListening, false  )
    QP_PTR_RW(QLlamaModel,      model               )
    QP_PTR_RW(QLlamaContext,    context             )
    QP_PTR_RW(QLlamaSampler,    sampler             )

public:
    explicit QLlamaServerEngine(QObject *parent = nullptr);
    ~QLlamaServerEngine() override;

    Q_INVOKABLE bool startServer();
    Q_INVOKABLE void stopServer();

private Q_SLOTS:
    void handleNewConnection();
    void handleClientReadyRead();
    void handleClientDisconnected();

private:
    void parseIncomingHttpRequest(QTcpSocket *clientSocket, const QByteArray &rawBuffer);
    void executeStreamingInference(QTcpSocket *clientSocket, const QString &promptPayload);

    QTcpServer *m_tcpServer = nullptr;
    QMap<QTcpSocket*, QByteArray> m_clientBuffers;

    bool m_isGenerating = false;
};

