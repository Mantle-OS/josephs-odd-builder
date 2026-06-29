#pragma once

#include <QObject>
#include <QUrl>
#include <QQueue>
#include <QHash>
#include <QFile>
#include <QElapsedTimer>
#include <QFuture>
#include <QPromise>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "property-macros.h"
#include "qaiutils.h"

class QDownloader : public QObject
{
    Q_OBJECT

    QP_RO(int,      current,            0)
    QP_RO(int,      total,              0)
    QP_RW(int,      maxConcurrent,      10)
    QP_RW(QString,  outDir,             QAiUtils::diffusionDir)
    QP_RO(qint64,   progressCurrent,    0)
    QP_RO(qint64,   progressMax,        0)
    QP_RO(QString,  speed,              "")

public:
    explicit QDownloader(QObject *parent = nullptr);
    ~QDownloader() noexcept override;
    QFuture<bool> append(const QUrl &url);
    QList<QFuture<bool>> append(const QList<QUrl> &urls);
    Q_INVOKABLE void cancelAll();

Q_SIGNALS:
    void finished();
    void itemStarted(const QUrl &url, const QString &fileName);
    void itemFinished(const QUrl &url, const QString &fileName);
    void itemFailed(const QUrl &url, const QString &errorString);

protected Q_SLOTS:
    void processQueue();
    void reqReadyRead();
    void reqProgress(qint64 received, qint64 total);
    void reqFinished();

protected:
    struct DownloadItem {
        QUrl url;
        QString finalFileName;
        QString partFileName;

        QFile *file = nullptr;
        QNetworkReply *reply = nullptr;

        QPromise<bool> promise;

        QElapsedTimer timer;
        qint64 received = 0;
        qint64 total = 0;

        bool cancelled = false;
    };

    QString saveFileName(const QUrl &url) const; // FIXME
    void startItem(DownloadItem *item);
    void finishItem(QNetworkReply *reply, bool ok, const QString &errorString = {});
    void cleanupItem(DownloadItem *item, bool removePartFile) noexcept;
    void updateAggregateProgress();

private:
    QNetworkAccessManager *m_nam = nullptr;

    QQueue<DownloadItem *> m_queue;
    QHash<QNetworkReply *, DownloadItem *> m_active;

    bool m_shuttingDown = false;
};