#include "qmlzstd.h"

QmlZstd::QmlZstd(QObject *parent) :
    QZstd{parent},
    m_signingKeys{new QmlSodiumKeys{this}},
    m_encryptionKey{new QmlSecureMem{this}}
{
    connect(m_signingKeys, &QmlSodiumKeys::publicKeyFileChanged,
            this, [this](const QString &path) {
                if (!setPublicKeyFile(path))
                    qWarning() << "[QmlZstd] Failed to set public key file:" << path;
            });

    connect(m_signingKeys, &QmlSodiumKeys::privateKeyFileChanged,
            this, [this](const QString &path) {
                if (!setPrivateKeyFile(path))
                    qWarning() << "[QmlZstd] Failed to set private key file (check it exists and matches the current public key):" << path;
            });
}

QmlZstd::~QmlZstd()
{
    if (m_encryptionKey && m_encryptionKey->mem())
        m_encryptionKey->mem()->clear();
}

QmlSecureMem *QmlZstd::get_encryptionKey() noexcept
{
    return m_encryptionKey;
}

bool QmlZstd::setEncryptionKey(QmlSecureMem *source)
{
    if (!source || !source->internalBuffer() || !m_encryptionKey)
        return false;

    QSecureMem *src = source->internalBuffer();
    QSecureMem *dst = m_encryptionKey->internalBuffer();

    if (!src || !dst || src->empty())
        return false;

    dst->clear();

    if (!dst->allocate(src->size()))
        return false;

    dst->copyFrom(src->data(), src->size());

    // QZstd::setPrivateKey is inherited, this is the real backend push.
    setPrivateKey(*dst);

    Q_EMIT encryptionKeyChanged();
    return true;
}

QmlSodiumKeys *QmlZstd::signingKeys() const noexcept
{
    return m_signingKeys;
}
