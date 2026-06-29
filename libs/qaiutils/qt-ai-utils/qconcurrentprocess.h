#ifndef QCONCURRENTPROCESS_H
#define QCONCURRENTPROCESS_H

#include <QObject>
#include <QQueue>
#include <QSemaphore>
#include <QFutureWatcher>

class QConcurrentProcess : public QObject {
    Q_OBJECT
public:
    explicit QConcurrentProcess(QObject *parent = nullptr);
    ~QConcurrentProcess() override;

    void addTaskToQueue(const QString &workDir, const QString &program, const QStringList &arguments);

Q_SIGNALS:
    void errorOccurred(const QString &program, const QString &message);
    void lastOutPut(const QString &program, const QString &output);
    void operationCompleted(const QString &program);

private:
    void processQueue();

    struct Task {
        QString workDir;
        QString program;
        QStringList arguments;
    };

    struct ResultData {
        QString program;
        int exitCode;
        QString output;
        bool success;
    };

    QQueue<Task> m_taskQueue;
    QSemaphore m_semaphore;

    // Track active execution instances dynamically to allow true concurrency
    QList<QFutureWatcher<ResultData>*> m_activeWatchers;
};

#endif // QCONCURRENTPROCESS_H