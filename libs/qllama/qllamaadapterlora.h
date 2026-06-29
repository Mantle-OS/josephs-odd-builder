#ifndef QLLAMAADAPTERLORA_H
#define QLLAMAADAPTERLORA_H

#include <QObject>
#include <QQmlEngine>
#include <llama.h>

#include <property-macros.h>
#include "qllamabase.h"
#include "qllamamodel.h"

class QLlamaAdapterLora : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT

    QP_RW(QString, loraPath, "")
    QP_RW(double,  scale,    1.0) // 1.0 = Full application intensity scaling
    QP_RO(bool,    isLoaded, false)

public:
    Q_INVOKABLE explicit QLlamaAdapterLora(QObject *parent = nullptr);
    ~QLlamaAdapterLora() override;
    Q_INVOKABLE bool loadAdapter(QLlamaModel *model);
    Q_INVOKABLE void unloadAdapter();
    struct llama_adapter_lora* nativeAdapter() const { return m_adapter; }

private:
    struct llama_adapter_lora *m_adapter = nullptr;
};

#endif // QLLAMAADAPTERLORA_H