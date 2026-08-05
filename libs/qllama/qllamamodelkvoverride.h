#pragma once

#include <QObject>
#include <QString>

#include <QQmlEngine>

#include <llama.h>

#include <property-macros.h>

#include "qllamabase.h"
#include "qllamaenums.h"
#include "qllama_export.h"

class QLLAMA_EXPORT QLlamaModelKvOverride : public QLlamaBase
{
    Q_OBJECT

    QP_RW(QLlamaEnums::QLlamaKvOverrideType, tag,     QLlamaEnums::QLlamaKvOverrideTypeInt ) // Native value type stored in the override union.
    QP_RW(QString,                           key,     ""                                    ) // GGUF metadata key overridden while loading the model.
    QP_RW(qint64,                            valI64,  0                                     ) // Signed 64-bit value used when tag is Int.
    QP_RW(double,                            valF64,  0.0                                   ) // Double-precision value used when tag is Float.
    QP_RW(bool,                              valBool, false                                 ) // Boolean value used when tag is Bool.
    QP_RW(QString,                           valStr,  ""                                    ) // UTF-8 value used when tag is Str; native storage is limited to 127 bytes.

    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(QLlamaModelKvOverride)

public:
    explicit QLlamaModelKvOverride(QObject *parent = nullptr);
    ~QLlamaModelKvOverride() override = default;

    void setModelKvOverride(const llama_model_kv_override &other);
    [[nodiscard]] llama_model_kv_override modelKvOverride();
    void resetModelKvOverride();

private:
    [[nodiscard]] static constexpr llama_model_kv_override defaultModelKvOverride() noexcept { return {LLAMA_KV_OVERRIDE_TYPE_INT, {}, {0}}; }
    llama_model_kv_override m_modelKvOverride{defaultModelKvOverride()};
};