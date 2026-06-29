#ifndef QLLAMAENGINE_H
#define QLLAMAENGINE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>

#include <property-macros.h>
#include "qllamabase.h"
#include "qllamavocab.h"

class QLlamaEngine : public QLlamaBase
{
    Q_OBJECT

    // Common across all engine environments
    QP_RW(QString, prompt,       "")
    QP_RW(QString, streamingText,"")
    QP_RO(bool,    isProcessing, false)

    QML_ELEMENT
    QML_UNCREATABLE("Abstract Orchestration Layer Base")
public:
    explicit QLlamaEngine(QObject *parent = nullptr) :
        QLlamaBase{parent}
    {
    }

    ~QLlamaEngine() override = default;
    Q_INVOKABLE virtual void generate() = 0;
    Q_INVOKABLE virtual void cancel() = 0;

Q_SIGNALS:
    void generationStarted();
    void tokenStreamed(const QString &textChunk);
    void generationFinished(const QString &fullCompletedText);
    void executionError(const QString &errorString);
};

#endif // QLLAMAENGINE_H