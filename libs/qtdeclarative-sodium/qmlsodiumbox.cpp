#include "qmlsodiumbox.h"
#include <qsodium.h>
#include <qsodiumpasswordutils.h>
#include <qsodiumsecretbox.h>
#include <qextrarandom.h>
#include <QDebug>

QmlSodiumBox::QmlSodiumBox(QObject *parent) :
    QObject{parent}
{
}

void QmlSodiumBox::generateNewSalt()
{
    QByteArray const rawSalt = QExtraRandom::randomSalt();
    set_salt(QString::fromLatin1(rawSalt.toBase64()));
}

bool QmlSodiumBox::encryptString(const QString &plainText)
{
    if (get_password().isEmpty()) {
        qWarning() << "[SodiumBox] Encryption failed: Password property is empty.";
        return false;
    }

    // If no salt has been specified yet by the UI component sheets, auto-generate one
    if (get_salt().isEmpty())
        generateNewSalt();

    QByteArray const saltBin = QByteArray::fromBase64(get_salt().toLatin1());
    QSecureMem derivedKey;

    if (!QSodiumPasswordUtils::deriveKeyFromPassword(derivedKey, get_password(), saltBin))
        return false;


    QByteArray cipherBin;
    QByteArray nonceBin;

    if (!QSodium::encryptConfig(plainText.toUtf8(), derivedKey, cipherBin, nonceBin))
        return false;


    set_cipherText(QString::fromLatin1(cipherBin.toBase64()));
    set_nonce(QString::fromLatin1(nonceBin.toBase64()));

    return true;
}

QString QmlSodiumBox::decryptToString()
{
    if (get_password().isEmpty() || get_salt().isEmpty() ||
        get_cipherText().isEmpty() || get_nonce().isEmpty()) {
        qWarning() << "[SodiumBox] Decryption aborted: Missing required parameter properties.";
        return {};
    }

    QByteArray const saltBin   = QByteArray::fromBase64(get_salt().toLatin1());
    QByteArray const cipherBin = QByteArray::fromBase64(get_cipherText().toLatin1());
    QByteArray const nonceBin  = QByteArray::fromBase64(get_nonce().toLatin1());

    QSecureMem derivedKey;
    if (!QSodiumPasswordUtils::deriveKeyFromPassword(derivedKey, get_password(), saltBin))
        return {};

    QSecureMem plainTextMem;
    if (!QSodium::decryptConfig(cipherBin, derivedKey, nonceBin, plainTextMem))
        return {};

    return QString::fromUtf8(reinterpret_cast<const char*>(plainTextMem.data()), plainTextMem.size());
}