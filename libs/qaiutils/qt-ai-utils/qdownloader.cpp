#include "qdownloader.h"
#include <QFileInfo>
#include <QDir>
QDownloader::QDownloader(QObject *parent) :
    QObject{parent},
    m_nam{new QNetworkAccessManager{this}}
{

}

QDownloader::~QDownloader() noexcept
{
    m_shuttingDown = true;

    while (!m_queue.isEmpty()) {
        DownloadItem *item = m_queue.dequeue();

        if (item) {
            item->cancelled = true;
            item->promise.addResult(false, 0);
            item->promise.finish();
            cleanupItem(item, true);
            delete item;
        }
    }

    const auto replies = m_active.keys();
    for (QNetworkReply *reply : replies) {
        if (!reply)
            continue;

        DownloadItem *item = m_active.value(reply, nullptr);
        if (item)
            item->cancelled = true;

        reply->abort();
    }

    // At destructor time we cannot rely on normal async finished delivery.
    // So force cleanup of anything still active.
    const auto remainingReplies = m_active.keys();
    for (QNetworkReply *reply : remainingReplies) {
        DownloadItem *item = m_active.take(reply);

        if (reply) {
            disconnect(reply, nullptr, this, nullptr);
            reply->deleteLater();
        }

        if (item) {
            item->promise.addResult(false, 0);
            item->promise.finish();
            cleanupItem(item, true);
            delete item;
        }
    }

    if (m_nam) {
        delete m_nam;
        m_nam = nullptr;
    }
}


QFuture<bool> QDownloader::append(const QUrl &url)
{
    QPromise<bool> promise;
    QFuture<bool> future = promise.future();

    auto *item = new DownloadItem;
    item->url = url;
    item->promise = std::move(promise);

    m_queue.enqueue(item);
    set_total(m_total + 1);

    if (!m_shuttingDown)
        QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);

    return future;
}

QList<QFuture<bool>> QDownloader::append(const QList<QUrl> &urls)
{
    QList<QFuture<bool>> futures;
    futures.reserve(urls.size());

    for (const QUrl &url : urls)
        futures.append(append(url));

    return futures;
}

void QDownloader::processQueue()
{
    if (m_shuttingDown)
        return;



    while (!m_queue.isEmpty() && m_active.size() < m_maxConcurrent) {
        DownloadItem *item = m_queue.dequeue();

        if (!item)
            continue;

        startItem(item);
    }

    if (m_queue.isEmpty() && m_active.isEmpty() && m_total > 0)
        Q_EMIT finished();
}

QString QDownloader::saveFileName(const QUrl &url) const
{
    QDir dir{m_outDir};

    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return {};

    QString baseName = url.fileName();

    if (baseName.isEmpty())
        baseName = QStringLiteral("download");

#if defined(JOB_WINDOWS)
    static const QString invalidChars = QStringLiteral("<>:\"/\\|?*");

    for (qsizetype i = 0; i < baseName.size(); ++i) {
        if (invalidChars.contains(baseName.at(i)))
            baseName[i] = QLatin1Char('_');
    }

    while (baseName.endsWith(QLatin1Char(' ')) || baseName.endsWith(QLatin1Char('.')))
        baseName.chop(1);

    if (baseName.isEmpty())
        baseName = QStringLiteral("download");
#endif

    QString candidate = dir.filePath(baseName);

    if (!QFile::exists(candidate) &&
        !QFile::exists(candidate + QStringLiteral(".part"))) {
        return candidate;
    }

    QFileInfo const info{baseName};
    QString const stem = info.completeBaseName();
    QString const suffix = info.suffix();

    for (int i = 1; ; ++i) {
        QString numbered;

        if (suffix.isEmpty())
            numbered = QStringLiteral("%1.%2").arg(stem).arg(i);
        else
            numbered = QStringLiteral("%1.%2.%3") .arg(stem) .arg(i) .arg(suffix);

        candidate = dir.filePath(numbered);

        if (!QFile::exists(candidate) &&
            !QFile::exists(candidate + QStringLiteral(".part"))) {
            return candidate;
        }
    }
}

void QDownloader::startItem(DownloadItem *item)
{
    if (!item)
        return;

    if (!m_nam) {
        item->promise.addResult(false, 0);
        item->promise.finish();
        delete item;
        return;
    }

    if (!item->url.isValid() || item->url.isEmpty()) {
        item->promise.addResult(false, 0);
        item->promise.finish();
        delete item;
        return;
    }

    item->finalFileName = saveFileName(item->url);
    item->partFileName = item->finalFileName + QStringLiteral(".part");

    item->file = new QFile(item->partFileName);

    if (!item->file->open(QIODevice::WriteOnly)) {
        const QString err = item->file->errorString();

        delete item->file;
        item->file = nullptr;

        item->promise.addResult(false, 0);
        item->promise.finish();

        Q_EMIT itemFailed(item->url, err);

        delete item;

        QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
        return;
    }

    QNetworkRequest req(item->url);

    req.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::NoLessSafeRedirectPolicy
        );

    req.setMaximumRedirectsAllowed(10);

    item->reply = m_nam->get(req);
    item->timer.start();

    m_active.insert(item->reply, item);

    connect(item->reply, &QNetworkReply::readyRead,
            this, &QDownloader::reqReadyRead);

    connect(item->reply, &QNetworkReply::downloadProgress,
            this, &QDownloader::reqProgress);

    connect(item->reply, &QNetworkReply::finished,
            this, &QDownloader::reqFinished);

    Q_EMIT itemStarted(item->url, item->finalFileName);
}

void QDownloader::reqReadyRead()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    DownloadItem *item = m_active.value(reply, nullptr);
    if (!item || !item->file)
        return;

    QByteArray const data = reply->readAll();
    if (data.isEmpty())
        return;

    qint64 const written = item->file->write(data);

    if (written != data.size()) {
        QString const errorString =
            item->file->errorString().isEmpty() ?
                                        QStringLiteral("Incomplete file write.") :
                                        item->file->errorString();

        finishItem(reply, false, errorString);
    }
}

void QDownloader::reqProgress(qint64 received, qint64 total)
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    DownloadItem *item = m_active.value(reply, nullptr);
    if (!item)
        return;

    item->received = received;
    item->total = total;

    updateAggregateProgress();
}


void QDownloader::reqFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    DownloadItem *item = m_active.value(reply, nullptr);
    if (!item) {
        reply->deleteLater();
        return;
    }

    if (item->cancelled) {
        finishItem(reply, false, QStringLiteral("Cancelled"));
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        finishItem(reply, false, reply->errorString());
        return;
    }

    finishItem(reply, true);
}


void QDownloader::finishItem(QNetworkReply *reply, bool ok, const QString &errorString)
{
    if (!reply)
        return;

    DownloadItem *item = m_active.take(reply);
    if (!item) {
        reply->deleteLater();
        return;
    }

    QString failureString = errorString;

    if (item->file) {
        if (!item->file->flush()) {
            ok = false;

            if (failureString.isEmpty())
                failureString = item->file->errorString();
        }

        item->file->close();

        if (item->file->error() != QFile::NoError) {
            ok = false;

            if (failureString.isEmpty())
                failureString = item->file->errorString();
        }
    }

    if (ok) {
        QFile::remove(item->finalFileName);

        if (!QFile::rename( item->partFileName, item->finalFileName )) {
            ok = false;
            failureString = QStringLiteral("Unable to move completed download into place.");
        }
    }

    if (ok) {
        item->promise.addResult(true, 0);
        item->promise.finish();
        Q_EMIT itemFinished(item->url, item->finalFileName);
    } else {
        QFile::remove(item->partFileName);
        item->promise.addResult(false, 0);
        item->promise.finish();

        Q_EMIT itemFailed(item->url, failureString);
    }

    cleanupItem(item, !ok);

    reply->deleteLater();

    delete item;

    set_current(m_current + 1);
    updateAggregateProgress();

    if (!m_shuttingDown)
        QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
}

void QDownloader::cleanupItem(DownloadItem *item, bool removePartFile) noexcept
{
    if (!item)
        return;

    if (item->reply) {
        disconnect(item->reply, nullptr, this, nullptr);

        if (item->reply->isRunning())
            item->reply->abort();

        item->reply = nullptr;
    }

    if (item->file) {
        if (item->file->isOpen()) {
            item->file->flush();
            item->file->close();
        }

        delete item->file;
        item->file = nullptr;
    }

    if (removePartFile && !item->partFileName.isEmpty())
        QFile::remove(item->partFileName);
}

void QDownloader::cancelAll()
{
    m_shuttingDown = true;

    while (!m_queue.isEmpty()) {
        DownloadItem *item = m_queue.dequeue();

        if (!item)
            continue;

        item->cancelled = true;
        item->promise.addResult(false, 0);
        item->promise.finish();

        cleanupItem(item, true);
        delete item;
    }

    const auto replies = m_active.keys();

    for (QNetworkReply *reply : replies) {
        DownloadItem *item = m_active.value(reply, nullptr);
        if (item)
            item->cancelled = true;

        if (reply)
            reply->abort();
    }
}

void QDownloader::updateAggregateProgress()
{
    qint64 received = 0;
    qint64 total = 0;
    double speedBytes = 0.0;

    for (DownloadItem *item : m_active) {
        if (!item)
            continue;

        received += item->received;

        if (item->total > 0)
            total += item->total;

        const qint64 elapsed = item->timer.elapsed();
        if (elapsed > 0)
            speedBytes += item->received * 1000.0 / double(elapsed);
    }

    set_progressCurrent(received);
    set_progressMax(total);

    QString unit = QStringLiteral("bytes/sec");
    double shown = speedBytes;

    if (shown >= 1024.0 * 1024.0) {
        shown /= 1024.0 * 1024.0;
        unit = QStringLiteral("MB/s");
    } else if (shown >= 1024.0) {
        shown /= 1024.0;
        unit = QStringLiteral("kB/s");
    }

    set_speed(QStringLiteral("%1 %2").arg(shown, 0, 'f', 1).arg(unit));
}