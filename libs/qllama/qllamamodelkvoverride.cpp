#include "qllamamodelkvoverride.h"

#include <cstring>

#include <QByteArray>

QLlamaModelKvOverride::QLlamaModelKvOverride(QObject *parent) :
    QLlamaBase{parent}
{

}

void QLlamaModelKvOverride::setModelKvOverride(const llama_model_kv_override &other)
{
    m_modelKvOverride = other;

    set_tag(QLlamaEnums::qLlamaKvOverrideType(other.tag));
    set_key(QString::fromUtf8(other.key));

    switch (other.tag) {
    case LLAMA_KV_OVERRIDE_TYPE_INT:
        set_valI64(other.val_i64);
        break;
    case LLAMA_KV_OVERRIDE_TYPE_FLOAT:
        set_valF64(other.val_f64);
        break;
    case LLAMA_KV_OVERRIDE_TYPE_BOOL:
        set_valBool(other.val_bool);
        break;
    case LLAMA_KV_OVERRIDE_TYPE_STR:
        set_valStr(QString::fromUtf8(other.val_str));
        break;
    }
}

llama_model_kv_override QLlamaModelKvOverride::modelKvOverride()
{
    llama_model_kv_override ret{defaultModelKvOverride()};

    ret.tag = QLlamaEnums::llamaKvOverrideType(get_tag());

    const QByteArray keyBytes = get_key().toUtf8();
    std::strncpy(ret.key, keyBytes.constData(), sizeof(ret.key) - 1);

    switch (ret.tag) {
    case LLAMA_KV_OVERRIDE_TYPE_INT:
        ret.val_i64 = get_valI64();
        break;
    case LLAMA_KV_OVERRIDE_TYPE_FLOAT:
        ret.val_f64 = get_valF64();
        break;
    case LLAMA_KV_OVERRIDE_TYPE_BOOL:
        ret.val_bool = get_valBool();
        break;
    case LLAMA_KV_OVERRIDE_TYPE_STR: {
        const QByteArray valueBytes = get_valStr().toUtf8();
        std::strncpy(ret.val_str, valueBytes.constData(), sizeof(ret.val_str) - 1);
        break;
    }
    }

    m_modelKvOverride = ret;
    return m_modelKvOverride;
}

void QLlamaModelKvOverride::resetModelKvOverride()
{
    m_modelKvOverride = defaultModelKvOverride();
    setModelKvOverride(m_modelKvOverride);
}