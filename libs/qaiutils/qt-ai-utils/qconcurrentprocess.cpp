#include "qconcurrentprocess.h"

#include <QProcess>
#include <QThread>
#include <QDebug>

#include <QtConcurrent/QtConcurrentRun>

QConcurrentProcess::QConcurrentProcess(QObject *parent) :
    QObject{parent},
    m_semaphore(qMax(1, QThread::idealThreadCount() - 1))
{

}

QConcurrentProcess::~QConcurrentProcess()
{
    for (auto *watcher : m_activeWatchers) {
        if (watcher->future().isRunning()) {
            watcher->waitForFinished();
        }
        delete watcher;
        watcher = nullptr;
    }
}

void QConcurrentProcess::addTaskToQueue(const QString &workDir,
                                        const QString &program,
                                        const QStringList &arguments)
{
    m_taskQueue.enqueue({workDir, program, arguments});
    processQueue();
}

void QConcurrentProcess::processQueue()
{
    while (!m_taskQueue.isEmpty() && m_semaphore.tryAcquire()) {
        Task task = m_taskQueue.dequeue();
        auto *watcher = new QFutureWatcher<ResultData>(this);
        QFuture<ResultData> future = QtConcurrent::run([task]() -> ResultData {
            ResultData res;
            res.program = task.program;
            res.exitCode = -1;
            res.success = false;

            QProcess process;
            process.setWorkingDirectory(task.workDir);
            process.setProgram(task.program);
            process.setArguments(task.arguments);
            process.setProcessChannelMode(QProcess::MergedChannels);
            process.start();
            if (!process.waitForStarted(5000)) {
                res.output = "Process failed to start within timeout.";
                return res;
            }

            if (!process.waitForFinished(-1)) {
                res.output = "Process hung or timed out during execution loop.";
                return res;
            }

            res.output = QString::fromUtf8(process.readAllStandardOutput());
            res.exitCode = process.exitCode();
            res.success = (process.exitStatus() == QProcess::NormalExit && res.exitCode == 0);

            return res;
        });

        connect(watcher, &QFutureWatcher<ResultData>::finished, this, [this, watcher]() {
            ResultData res = watcher->result();
            Q_EMIT lastOutPut(res.program, res.output);

            if (!res.success)
                Q_EMIT errorOccurred(res.program, QString("Code: %1").arg(res.exitCode));
            else
                Q_EMIT operationCompleted(res.program);

            m_activeWatchers.removeOne(watcher);
            watcher->deleteLater();

            m_semaphore.release();
            processQueue();
        });

        m_activeWatchers.append(watcher);
        watcher->setFuture(future);
    }
}