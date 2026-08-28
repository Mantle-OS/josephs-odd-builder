#include "qmlsodiumhmacsha256.h"

#include <QByteArray>
#include <QDebug>

SodiumHmacSha256::SodiumHmacSha256(QObject *parent) :
    QObject{parent},
    m_key{new QmlSecureMem{this}}
{
    set_hasKey(false);
    set_valid(false);
}

SodiumHmacSha256::~SodiumHmacSha256()
{
    clearKey();
}

QmlSecureMem *SodiumHmacSha256::key() const noexcept
{
    return m_key;
}

bool SodiumHmacSha256::setKey(QmlSecureMem *key) noexcept
{
    if (!key || !key->internalBuffer() || !m_key || !m_key->internalBuffer()) {
        qWarning() << "[SodiumHmacSha256] Missing key memory.";
        return false;
    }

    QSecureMem *src = key->internalBuffer();
    QSecureMem *dst = m_key->internalBuffer();

    if (src->empty()) {
        qWarning() << "[SodiumHmacSha256] Source key memory is empty.";
        return false;
    }

    dst->clear();

    if (!dst->allocate(src->size())) {
        qWarning() << "[SodiumHmacSha256] Failed to allocate key memory.";
        return false;
    }

    dst->copyFrom(src->data(), src->size());

    if (m_mac != "unknown")
        set_lastMac(m_mac);

    set_mac("unknown");

    Q_EMIT keyChanged(m_key);

    isValid();
    return m_hasKey;
}

void SodiumHmacSha256::clearKey() noexcept
{
    if (m_mac != "unknown")
        set_lastMac(m_mac);

    if (m_key && m_key->internalBuffer())
        m_key->internalBuffer()->clear();

    set_mac("unknown");

    Q_EMIT keyChanged(m_key);

    isValid();
}

QString SodiumHmacSha256::compute(const QString &data) noexcept
{
    if (!hasKey()) {
        qWarning() << "[SodiumHmacSha256] Cannot compute MAC without a valid key.";
        isValid();
        return {};
    }

    QByteArray const rawMac = QSodiumHmacSha256::compute(
        data.toUtf8(),
        *m_key->internalBuffer()
        );

    if (rawMac.isEmpty()) {
        qWarning() << "[SodiumHmacSha256] HMAC-SHA256 computation failed.";
        isValid();
        return {};
    }

    QString const currentMac = QString::fromLatin1(rawMac.toHex());

    if (m_mac != "unknown")
        set_lastMac(m_mac);

    set_mac(currentMac);

    isValid();

    return currentMac;
}

bool SodiumHmacSha256::verify(const QString &data) noexcept
{
    if (!isValid())
        return false;

    QByteArray const rawMac = QByteArray::fromHex(m_mac.toLatin1());

    return QSodiumHmacSha256::verify(
        rawMac,
        data.toUtf8(),
        *m_key->internalBuffer()
        );
}

bool SodiumHmacSha256::generateKey() noexcept
{
    if (!m_key || !m_key->internalBuffer()) {
        qWarning() << "[SodiumHmacSha256] Missing destination key memory.";
        return false;
    }

    QSecureMem generated = QSodiumHmacSha256::generateKey();

    if (generated.empty()) {
        qWarning() << "[SodiumHmacSha256] Failed to generate HMAC-SHA256 key.";
        return false;
    }

    QSecureMem *dst = m_key->internalBuffer();

    dst->clear();

    if (!dst->allocate(generated.size())) {
        qWarning() << "[SodiumHmacSha256] Failed to allocate generated key memory.";
        return false;
    }

    dst->copyFrom(generated.data(), generated.size());

    if (m_mac != "unknown")
        set_lastMac(m_mac);

    set_mac("unknown");

    Q_EMIT keyChanged(m_key);

    isValid();
    return m_hasKey;
}

bool SodiumHmacSha256::hasKey() const noexcept
{
    return m_key &&
           m_key->internalBuffer() &&
           !m_key->internalBuffer()->empty();
}

bool SodiumHmacSha256::isValid() noexcept
{
    bool const keyAvailable = hasKey();

    set_hasKey(keyAvailable);

    bool const val =
        keyAvailable &&
        !m_mac.isEmpty() &&
        m_mac != "unknown";

    set_valid(val);

    if (!val)
        set_mac("unknown");

    return m_valid;
}