#include "qmlsodiumbox.h"
#include <qsodium.h>
#include <qsodiumpasswordutils.h>
#include <qextrarandom.h>
#include <QDebug>
QmlSodiumBox::QmlSodiumBox(QObject *parent) :
    QObject{parent},
    m_password{new QmlSecureMem{this}}
{

}
QmlSodiumBox::~QmlSodiumBox()
{
    if (m_password)
        m_password->mem()->clear();

}

QmlSecureMem *QmlSodiumBox::get_password() noexcept
{
    return m_password;
}

bool QmlSodiumBox::setPassword(QmlSecureMem *source) noexcept
{
    return copyPasswordMemoryFrom(source);
}

void QmlSodiumBox::generateNewSalt()
{
    QByteArray const rawSalt = QExtraRandom::randomSalt();
    set_salt(QString::fromLatin1(rawSalt.toBase64()));
}

bool QmlSodiumBox::deriveKey(QSecureMem &derivedKey) noexcept
{
    if (!m_password || !m_password->internalBuffer()) {
        qWarning() << "[SodiumBox] Missing password memory.";
        return false;
    }

    QSecureMem *passwordMem = m_password->internalBuffer();
    if (!passwordMem || passwordMem->empty()) {
        qWarning() << "[SodiumBox] Password memory is empty.";
        return false;
    }

    if (get_salt().isEmpty())
        generateNewSalt();

    QByteArray const saltBin = QByteArray::fromBase64(get_salt().toLatin1());
    if (saltBin.isEmpty()) {
        qWarning() << "[SodiumBox] Salt decode failed.";
        return false;
    }

    return QSodiumPasswordUtils::deriveKeyFromPassword(
        derivedKey,
        *passwordMem,
        saltBin
        );
}

void QmlSodiumBox::set_password(QmlSecureMem *source) noexcept
{
    if (!source || !source->internalBuffer() || ! m_password)
        return;

    QSecureMem *src = source->internalBuffer();

    if (!src || src->size() == 0)
        return;

    if (! m_password->copyFromSecureMem(*src))
        return;

    Q_EMIT passwordChanged( m_password);
}

bool QmlSodiumBox::copyPasswordMemoryFrom(QmlSecureMem *source) noexcept
{
    if (!source || !source->internalBuffer() || ! m_password)
        return false;

    QSecureMem *src = source->internalBuffer();
    QSecureMem *dst =  m_password->internalBuffer();

    if (!src || !dst || src->empty())
        return false;

    dst->clear();

    if (!dst->allocate(src->size()))
        return false;

    dst->copyFrom(src->data(), src->size());

    Q_EMIT passwordChanged(m_password);

    return true;
}

bool QmlSodiumBox::encryptString(const QString &plainText)
{
    QSecureMem derivedKey;

    if (!deriveKey(derivedKey))
        return false;

    QByteArray cipherBin;
    QByteArray nonceBin;

    if (!QSodium::encryptConfig(plainText.toUtf8(), derivedKey, cipherBin, nonceBin)) {
        qWarning() << "[SodiumBox] Encryption failed.";
        return false;
    }

    set_cipherText(QString::fromLatin1(cipherBin.toBase64()));
    set_nonce(QString::fromLatin1(nonceBin.toBase64()));

    return true;
}

QString QmlSodiumBox::decryptToString()
{
    if (get_cipherText().isEmpty() || get_nonce().isEmpty()) {
        qWarning() << "[SodiumBox] Decryption aborted: missing cipherText or nonce.";
        return {};
    }

    QByteArray const cipherBin = QByteArray::fromBase64(get_cipherText().toLatin1());
    QByteArray const nonceBin  = QByteArray::fromBase64(get_nonce().toLatin1());

    if (cipherBin.isEmpty() || nonceBin.isEmpty()) {
        qWarning() << "[SodiumBox] Decryption aborted: invalid cipherText or nonce.";
        return {};
    }

    QSecureMem derivedKey;

    if (!deriveKey(derivedKey))
        return {};

    QSecureMem plainTextMem;

    if (!QSodium::decryptConfig(cipherBin, derivedKey, nonceBin, plainTextMem)) {
        qWarning() << "[SodiumBox] Decryption failed.";
        return {};
    }

    return QString::fromUtf8(
        reinterpret_cast<const char *>(plainTextMem.data()),
        static_cast<int>(plainTextMem.size())
        );
}