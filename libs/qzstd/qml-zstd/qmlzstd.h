#pragma once

#include <QObject>
#include <QDebug>
#include <qqmlregistration.h>

#include <qmlsecuremem.h>
#include <qmlsodiumkeys.h>
#include <qzstd.h>

class QmlZstd : public QZstd
{
    Q_OBJECT
    Q_PROPERTY(QmlSodiumKeys *signingKeys READ signingKeys CONSTANT)
    Q_PROPERTY(QmlSecureMem *encryptionKey READ get_encryptionKey NOTIFY encryptionKeyChanged)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QmlZstd(QObject *parent = nullptr) :
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

    ~QmlZstd() override
    {
        if (m_encryptionKey && m_encryptionKey->mem())
            m_encryptionKey->mem()->clear();
    }

    QmlSodiumKeys *signingKeys() const noexcept { return m_signingKeys; }
    QmlSecureMem *get_encryptionKey() noexcept { return m_encryptionKey; }

    Q_INVOKABLE bool setEncryptionKey(QmlSecureMem *source) noexcept
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

Q_SIGNALS:
    void encryptionKeyChanged();

private:
    QmlSodiumKeys *m_signingKeys = nullptr;   // owned, parented to this
    QmlSecureMem *m_encryptionKey = nullptr;  // owned, parented to this
};

