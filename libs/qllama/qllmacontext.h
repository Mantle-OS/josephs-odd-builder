#ifndef QLLAMACONTEXT_H
#define QLLAMACONTEXT_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QList>

#include <llama.h>

#include <property-macros.h>
#include <pointer-macros.h>
#include <objectmodel.h>

#include "qllamabase.h"
#include "qllamamodel.h"
#include "qllamacontextparams.h"
#include "qllamaadapterlora.h"
#include "qllamasampler.h"


class QLlamaContext : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT

    QP_RO(bool,    isActive, false)
    QP_RO(qint32,  kvTokensUsed, 0)

    // Declarative read-only list model tracking active dynamic fine-tuning layers
    QP_PTR_RO(ObjectListModel<QLlamaAdapterLora>, loraModel)

public:
    Q_INVOKABLE explicit QLlamaContext(QObject *parent = nullptr);
    ~QLlamaContext() override;

    // Direct lifecycle initialization entry points
    Q_INVOKABLE bool initContext(QLlamaModel *model, QLlamaContextParams *params);
    Q_INVOKABLE void releaseContext();

    // Context Modification Mechanics
    Q_INVOKABLE void clearKvCache();
    Q_INVOKABLE bool syncActiveLoRAs();

    // "High performance" synchronous evaluation primitives
    Q_INVOKABLE int sampleNextToken(QLlamaSampler *sampler, const QList<int> &inputTokens);

    // Opaque handle accessor
    struct llama_context* nativeContext() const { return m_context; }

Q_SIGNALS:
    void contextCreated();
    void contextDestroyed();
    void kvCacheReset();

private:
    struct llama_context *m_context = nullptr;
    QLlamaModel  *m_linkedModel = nullptr;

    // Internal batch manager allocations tracking structural multi-token states
    llama_batch m_batch;
    bool m_batchAllocated = false;
};


#endif // QLLAMACONTEXT_H