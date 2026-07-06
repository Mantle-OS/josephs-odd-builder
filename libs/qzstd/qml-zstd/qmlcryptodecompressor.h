// qmlcryptodecompressor.h
#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include <qmlsecuremem.h>
#include <qextrarandom.h>
#include <qsodiumpasswordutils.h>
#include <qzstdoptions.h>
#include <qzstddecompressorcrypto.h>

class QmlCryptoDecompressor : public QZstdOptions
{
    Q_OBJECT
    Q_PROPERTY(QmlSecureMem *password READ get_password NOTIFY passwordChanged)
    QP_RW(QString, salt, "")
    QML_ELEMENT

public:
    explicit QmlCryptoDecompressor(QObject *parent = nullptr) :
        QZstdOptions{parent},
        m_password{new QmlSecureMem{this}}
    {
        m_dec = new QZstdDecompressorCrypto{this};
    }

    ~QmlCryptoDecompressor()
    {
        if (m_password && m_password->mem())
            m_password->mem()->clear();

        delete m_dec;
        m_dec = nullptr;
    }

    Q_INVOKABLE bool decompress()
    {
        QSecureMem derivedKey;
        if (!deriveKey(derivedKey))
            return false;

        m_dec->setDecryptionKey(derivedKey);
        return m_dec->decryptAndDecompress();
    }

    QmlSecureMem *get_password() noexcept { return m_password; }

    Q_INVOKABLE bool setPassword(QmlSecureMem *source) noexcept
    {
        return copyPasswordMemoryFrom(source);
    }

Q_SIGNALS:
    void passwordChanged();

private:
    bool deriveKey(QSecureMem &derivedKey) noexcept
    {
        if (!m_password || !m_password->internalBuffer() || m_password->internalBuffer()->empty())
            return false;

        if (get_salt().isEmpty())
            return false;

        QByteArray const saltBin = QByteArray::fromBase64(get_salt().toLatin1());
        if (saltBin.isEmpty())
            return false;

        return QSodiumPasswordUtils::deriveKeyFromPassword(derivedKey, *m_password->internalBuffer(), saltBin);
    }

    bool copyPasswordMemoryFrom(QmlSecureMem *source) noexcept
    {
        if (!source || !source->internalBuffer() || !m_password)
            return false;

        QSecureMem *src = source->internalBuffer();
        QSecureMem *dst = m_password->internalBuffer();

        if (!src || !dst || src->empty())
            return false;

        dst->clear();

        if (!dst->allocate(src->size()))
            return false;

        dst->copyFrom(src->data(), src->size());
        Q_EMIT passwordChanged();
        return true;
    }

private:
    QZstdDecompressorCrypto *m_dec = nullptr;
    QmlSecureMem *m_password = nullptr;
};