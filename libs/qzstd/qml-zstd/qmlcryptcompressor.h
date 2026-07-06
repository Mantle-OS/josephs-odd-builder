#pragma once
#include <QObject>
#include <qqmlintegration.h>

#include <qmlsecuremem.h>
#include <qextrarandom.h>
#include <qsodiumpasswordutils.h>

#include "qzstdoptions.h"
#include "qzstdcompressorcrypto.h"

class QmlCyptoCompressor : public QZstdOptions
{
    Q_OBJECT
    Q_PROPERTY(QmlSecureMem *password READ get_password NOTIFY passwordChanged)
    QP_RW(QString,          salt,                                           "")
    QML_ELEMENT

public:
    explicit QmlCyptoCompressor(QObject *parent = nullptr) :
        QZstdOptions{parent},
        m_password{new QmlSecureMem{this}}
    {
        m_comp = new QZstdCompressorCrypto{this};
    }

    ~QmlCyptoCompressor()
    {
        if (m_password && m_password->mem())
            m_password->mem()->clear();

        delete m_comp;
        m_comp = nullptr;
    }

    Q_INVOKABLE bool compress(bool autoSalt = false)
    {

        if (autoSalt || get_salt().isEmpty())
            generateNewSalt();

        QSecureMem derivedKey;
        if (!deriveKey(derivedKey))
            return false;


        m_comp->setEncryptionKey(derivedKey);
        return m_comp->compressAndEncrypt();
    }

    QmlSecureMem *get_password() noexcept { return m_password; }
    Q_INVOKABLE bool setPassword(QmlSecureMem *source) noexcept
    {
        return copyPasswordMemoryFrom(source);
    }

    Q_INVOKABLE void generateNewSalt()
    {
        QByteArray const rawSalt = QExtraRandom::randomSalt();
        set_salt(QString::fromLatin1(rawSalt.toBase64()));
    }
Q_SIGNALS:
    void passwordChanged();

private:
    bool deriveKey(QSecureMem &derivedKey) noexcept
    {
        if (!m_password || !m_password->internalBuffer() || m_password->internalBuffer()->empty())
            return false;

        if (get_salt().isEmpty())
            generateNewSalt();

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
    QZstdCompressorCrypto   *m_comp = nullptr;
    QmlSecureMem            *m_password = nullptr;
};