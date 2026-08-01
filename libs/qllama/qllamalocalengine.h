#pragma once

#include "qllamaengine.h"
#include "qllamamodel.h"
#include "qllmacontext.h"
#include "qllamasampler.h"

#include <QThread>
#include <atomic>

// Dedicated, non-blocking background processor tracking generation states very alpha
class QLlamaLocalWorker : public QObject
{
    Q_OBJECT
public:
    explicit QLlamaLocalWorker(QObject *parent = nullptr) :
        QObject{parent},
        m_abort(false)
    {}

    void setParams(QLlamaModel *model, QLlamaContext *ctx, QLlamaSampler *sampler, const QString &prompt) {
        m_model = model;
        m_context = ctx;
        m_sampler = sampler;
        m_prompt = prompt;
    }

    void requestAbort() { m_abort.store(true); }

public Q_SLOTS:
    void process();

Q_SIGNALS:
    void tokenGenerated(const QString &text);
    void finished(const QString &fullText);
    void errorOccurred(const QString &err);

private:
    QLlamaModel   *m_model   = nullptr;
    QLlamaContext *m_context = nullptr;
    QLlamaSampler *m_sampler = nullptr;
    QString        m_prompt;
    std::atomic<bool> m_abort;
};

// Master Local Orchestrator managing worker lifecycles and QML hooks
class QLlamaLocalEngine : public QLlamaEngine
{
    Q_OBJECT
    QML_ELEMENT

    QP_PTR_RW(QLlamaModel,   model)
    QP_PTR_RW(QLlamaContext, context)
    QP_PTR_RW(QLlamaSampler, sampler)

public:
    explicit QLlamaLocalEngine(QObject *parent = nullptr);
    ~QLlamaLocalEngine() override;

    void generate() override;
    void cancel() override;

private:
    QThread            *m_workerThread = nullptr;
    QLlamaLocalWorker  *m_worker       = nullptr;
};
