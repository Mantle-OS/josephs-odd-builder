#ifndef QLLAMAMODEL_H
#define QLLAMAMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <llama.h>

#include <property-macros.h>
#include "qllamabase.h"
#include "qllamamodelparams.h"

class QLlamaModel : public QLlamaBase
{
    Q_OBJECT
    QP_RW(QString, modelPath,    "")
    QP_RO(bool,    isLoaded,     false)
    QP_RO(quint64, tensorCount,  0)
    QP_RO(quint64, parameterCount, 0)
    QP_RO(QString, architecture, "unknown")
    QML_ELEMENT

public:
    Q_INVOKABLE explicit QLlamaModel(QObject *parent = nullptr);
    ~QLlamaModel() override;

    // Direct initialization entry point from QML or execution runners
    Q_INVOKABLE bool loadModel(QLlamaModelParams *params);
    Q_INVOKABLE void unloadModel();

    struct llama_model* nativeModel() const { return m_model; }
    const struct llama_vocab* nativeVocab() const { return m_vocab; }

Q_SIGNALS:
    void modelLoaded();
    void modelUnloaded();

private:
    struct llama_model *m_model = nullptr;
    const struct llama_vocab *m_vocab = nullptr; // llama_model owns this lifecycle pointer internally
};

#endif // QLLAMAMODEL_H