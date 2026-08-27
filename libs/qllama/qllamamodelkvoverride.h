#pragma once
#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <cstring>
#include <llama.h>

#include <property-macros.h>
#include "qllamabase.h"
#include "qllamaenums.h"
#include "qllama_export.h"

class QLLAMA_EXPORT  QLlamaModelKvOverride : public QLlamaBase
{
    Q_OBJECT

    QP_RW(QLlamaEnums::QLlamaKvOverrideType,    tag,        QLlamaEnums::QLlamaKvOverrideTypeInt    )
    QP_RW(QString,                              key,        ""                                      )
    QP_RW(qint64,                               valI64,     0                                       )
    QP_RW(double,                               valF64,     0.0                                     )
    QP_RW(bool,                                 valBool,    false                                   )
    QP_RW(QString,                              valStr,     ""                                      )

    QML_ELEMENT

public:
    explicit QLlamaModelKvOverride(QObject *parent = nullptr) :
        QLlamaBase{parent},
        m_modelKvOverride{}
    {
        connect(this, &QLlamaModelKvOverride::tagChanged, this, [this](const QLlamaEnums::QLlamaKvOverrideType &val){ m_modelKvOverride.tag = QLlamaEnums::llamaKvOverrideType(val); });
        connect(this, &QLlamaModelKvOverride::keyChanged, this, [this](const QString &val){
            std::memset(m_modelKvOverride.key, 0, sizeof(m_modelKvOverride.key));
            QByteArray bytes = val.toUtf8();
            std::strncpy(m_modelKvOverride.key, bytes.constData(), sizeof(m_modelKvOverride.key) - 1);
        });
        connect(this, &QLlamaModelKvOverride::valI64Changed, this, [this](const qint64 &val){ m_modelKvOverride.val_i64 = val; });
        connect(this, &QLlamaModelKvOverride::valF64Changed, this, [this](const double &val){ m_modelKvOverride.val_f64 = val; });
        connect(this, &QLlamaModelKvOverride::valBoolChanged, this, [this](const bool &val){  m_modelKvOverride.val_bool = val; });

        connect(this, &QLlamaModelKvOverride::valStrChanged, this, [this](const QString &val){
            std::memset(m_modelKvOverride.val_str, 0, sizeof(m_modelKvOverride.val_str));
            QByteArray bytes = val.toUtf8();
            std::strncpy(m_modelKvOverride.val_str, bytes.constData(), sizeof(m_modelKvOverride.val_str) - 1);
        });
    }

    ~QLlamaModelKvOverride() override = default;
    llama_model_kv_override nativeOverride() const { return m_modelKvOverride; }

private:
    llama_model_kv_override m_modelKvOverride;
};